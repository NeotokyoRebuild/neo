//-----------------------------------------------------------------------------
// VAudio_MiniMP3
// 
// Created by Joshua Ashton and Josh Dowell (Slartibarty)
// joshua@froggi.es 🐸✨
//-----------------------------------------------------------------------------

#include "tier0/basetypes.h"
#include "tier0/dbg.h"
#include "tier1/interface.h"

#include "vaudio/ivaudio.h"

#define MINIMP3_ONLY_MP3
#define MINIMP3_ONLY_SIMD
//#define MINIMP3_NO_SIMD
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

// Each chunk is 4096 bytes because that is the size of AUDIOSOURCE_COPYBUF_SIZE
// which is used when streaming in CWaveDataStreamAsync::ReadSourceData
// and going above it's gives us garbage data back.
static const int kChunkSize    = 4096;
// 4 chunks of 4KB to make 16KB, or at least 10 frames.
// The buffer we are always called to fill is always 16KB,
// so this will ensure we will always saturate that buffer unless
// we reach the EOF even in the case we need to re-sync if we
// somehow got misaligned, our position got force set and there was garbage
// data in the stream, etc.
static const int kChunkCount   = 4;

//-----------------------------------------------------------------------------
// Implementation of IAudioStream
//-----------------------------------------------------------------------------
class CMiniMP3AudioStream final : public IAudioStream
{
public:
	CMiniMP3AudioStream( IAudioStreamEvent *pEventHandler );

	int				Decode( void *pBuffer, unsigned int uBufferSize ) override;

	int				GetOutputBits() override;
	int				GetOutputRate() override;
	int				GetOutputChannels() override;

	unsigned int	GetPosition() override;

	void			SetPosition( unsigned int uPosition ) override;

private:

	void			UpdateStreamInfo();
	// Returns true if it hit EOF
	bool 			StreamChunk( int nChunkIdx );
	// Returns number of samples
	int				DecodeFrame( void *pBuffer );

	unsigned int	SamplesToBytes( int nSamples ) const;
	unsigned int	GetTotalChunkSizes() const;

	mp3dec_t				m_Decoder;
	mp3dec_frame_info_t		m_Info;

	// Diagram of how the chunk system below fits into
	// a mp3 data stream.
	// The 'frame' cursor is local to the chunks.
	// The 'data' cursor is how far along we are in picking
	// up chunks.
	//----------------------------------------------------
	//      | chunk 1 | chunk 2 | chunk 3 | chunk 4 |
	//----------------------------------------------------
	//              ^                               ^
	//            frame                           data

	// Position of the 'data' cursor, used to fill
	// m_Frames.
	unsigned int 			m_uDataPosition = 0;
	// Position of the 'frame' cursor, inside of
	// m_Frames.
	unsigned int			m_uFramePosition = 0;

	IAudioStreamEvent*		m_pEventHandler;

	// Buffers for the current frames.
	// See comments describing the chunk size relationship at
	// the definition of kChunkSize and kChunkCount.
	union
	{
		uint8_t				m_FullFrame	[kChunkSize * kChunkCount];
		uint8_t				m_Chunks	[kChunkCount][kChunkSize];
	} m_Frames;

	int m_nChunkSize[kChunkCount] = { 0 };

	unsigned int m_nEOFPosition = ~0u;
};


CMiniMP3AudioStream::CMiniMP3AudioStream( IAudioStreamEvent *pEventHandler )
	: m_pEventHandler( pEventHandler )
{
	mp3dec_init( &m_Decoder );

	memset( &m_Info, 0, sizeof( m_Info ) );
	memset( &m_Frames, 0, sizeof( m_Frames ) );
	
	UpdateStreamInfo();
}


int	CMiniMP3AudioStream::Decode( void *pBuffer, unsigned int uBufferSize )
{
	const unsigned int kSamplesPerFrameBufferSize = MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof( short );

	if ( uBufferSize < kSamplesPerFrameBufferSize )
	{
		AssertMsg( false, "Decode called with < kSamplesPerFrameBufferSize!" );
		return 0;
	}

	unsigned int uSampleBytes = 0;
	while ( ( uBufferSize - uSampleBytes ) > kSamplesPerFrameBufferSize )
	{
		// Offset the buffer by the number of samples bytes we've got so far.
		char *pFrameBuffer = reinterpret_cast<char*>( pBuffer ) + uSampleBytes;

		int nFrameSamples = DecodeFrame( pFrameBuffer );

		if ( !nFrameSamples )
			break;

		uSampleBytes += SamplesToBytes( nFrameSamples );
	}

	// If we got no samples back and didnt hit EOF,
	// don't return 0 because it still end playback.
	//
	// If this is a streaming MP3, this is just from judder,
	// so just fill with 1152 samples of 0.
	const bool bEOF = m_uDataPosition >= m_nEOFPosition;
	if ( uSampleBytes == 0 && !bEOF )
	{
		const unsigned int kZeroSampleBytes = Max( SamplesToBytes( 1152 ), uBufferSize );
		memset( pBuffer, 0, kZeroSampleBytes );
		return kZeroSampleBytes;
	}

	return int( uSampleBytes );
}


int CMiniMP3AudioStream::GetOutputBits()
{
	// Unused, who knows what it's supposed to return.
	return m_Info.bitrate_kbps;
}


int CMiniMP3AudioStream::GetOutputRate()
{
	return m_Info.hz;
}


int CMiniMP3AudioStream::GetOutputChannels()
{
	// Must return at least 1 in an error state
	// or the engine will do a nasty div by 0.
	return Max( m_Info.channels, 1 );
}


unsigned int CMiniMP3AudioStream::GetPosition()
{
	// Current position is ( our data position - size of our cached chunks ) + position inside of them.
	return ( m_uDataPosition - sizeof( m_Frames ) ) + m_uFramePosition;
}

void CMiniMP3AudioStream::SetPosition( unsigned int uPosition )
{
	m_uDataPosition = uPosition;
	m_uFramePosition = 0;

	UpdateStreamInfo();
}


void CMiniMP3AudioStream::UpdateStreamInfo()
{
	// Pre-fill all frames.
	for ( int i = 0; i < kChunkCount; i++ )
	{
		const bool bEOF = StreamChunk( i );
		if ( bEOF )
		{
			m_nEOFPosition = m_uDataPosition;
			break;
		}
	}

	// Decode a frame to get the latest info, maybe we transitioned from
	// stereo <-> mono, etc.
	mp3dec_decode_frame( &m_Decoder, &m_Frames.m_FullFrame[ m_uFramePosition ], GetTotalChunkSizes() - m_uFramePosition, nullptr, &m_Info );
}


bool CMiniMP3AudioStream::StreamChunk( int nChunkIdx )
{
	m_nChunkSize[nChunkIdx] = m_pEventHandler->StreamRequestData( &m_Frames.m_Chunks[ nChunkIdx ], kChunkSize, m_uDataPosition );

	m_uDataPosition += m_nChunkSize[nChunkIdx];

	// Check if we hit EOF (ie. chunk size != max) and mark the EOF position
	// so we know when to stop playing.
	return m_nChunkSize[nChunkIdx] != kChunkSize;
}


int CMiniMP3AudioStream::DecodeFrame( void *pBuffer )
{
	// If we are past the first two chunks, move those two chunks back and load two new ones.
	//
	// This part of the code assumes the chunk count to be 4, so if you change that,
	// check here. You shouldn't need more than 4 4KB chunks making 16KB though...
	COMPILE_TIME_ASSERT( kChunkCount == 4 );
	while ( m_uFramePosition >= 2 * kChunkSize && m_uDataPosition < m_nEOFPosition )
	{
		// Chunk 0 <- Chunk 2
		// Chunk 1 <- Chunk 3
		memcpy( &m_Frames.m_Chunks[0], &m_Frames.m_Chunks[2], kChunkSize );
		memcpy( &m_Frames.m_Chunks[1], &m_Frames.m_Chunks[3], kChunkSize );
		m_nChunkSize[0] = m_nChunkSize[2];
		m_nChunkSize[1] = m_nChunkSize[3];
		m_nChunkSize[2] = 0;
		m_nChunkSize[3] = 0;

		// Move our frame position back by two chunks
		m_uFramePosition -= 2 * kChunkSize;

		// Grab a new Chunk 2 + 3
		for ( int i = 0; i < 2; i++ )
		{
			const int nChunkIdx = 2 + i;

			// StreamChunk returns if we hit EOF.
			const bool bEOF = StreamChunk( nChunkIdx );

			// If we did hit EOF, break out here, cause we don't need
			// to get the next chunk if there is one left to get.
			// It's okay if it never gets data, as m_nChunkSize[3] is set to 0
			// when we move the chunks back.
			if ( bEOF )
			{
				m_nEOFPosition = m_uDataPosition;
				break;
			}
		}
	}

	int nSamples = mp3dec_decode_frame( &m_Decoder, &m_Frames.m_FullFrame[ m_uFramePosition ], GetTotalChunkSizes() - m_uFramePosition, reinterpret_cast< short* >( pBuffer ), &m_Info );

	m_uFramePosition += m_Info.frame_bytes;

	return nSamples;
}


unsigned int CMiniMP3AudioStream::SamplesToBytes( int nSamples ) const
{
	return nSamples * sizeof( short ) * m_Info.channels;
}


unsigned int CMiniMP3AudioStream::GetTotalChunkSizes() const
{
	int nTotalSize = 0;
	for ( int i = 0; i < kChunkCount; i++ )
		nTotalSize += m_nChunkSize[i];

	return nTotalSize;
}


//-----------------------------------------------------------------------------
// Implementation of IVAudio
//-----------------------------------------------------------------------------
class CVAudioMiniMP3 final : public IVAudio
{
public:
	IAudioStream*	CreateMP3StreamDecoder ( IAudioStreamEvent *pEventHandler ) override;
	void			DestroyMP3StreamDecoder( IAudioStream *pDecoder ) override;

#ifdef GAME_DESOLATION
	void*			CreateMilesAudioEngine() override;
	void			DestroyMilesAudioEngine( void *pEngine ) override;
#endif

	static CVAudioMiniMP3 &GetInstance();
};


IAudioStream *CVAudioMiniMP3::CreateMP3StreamDecoder( IAudioStreamEvent *pEventHandler )
{
	return new CMiniMP3AudioStream( pEventHandler );
}


void CVAudioMiniMP3::DestroyMP3StreamDecoder( IAudioStream *pDecoder )
{
	delete static_cast< CMiniMP3AudioStream * >( pDecoder );
}


#ifdef GAME_DESOLATION
void *CVAudioMiniMP3::CreateMilesAudioEngine()
{
	// Only used for Bink videos
	return nullptr;
}
void CVAudioMiniMP3::DestroyMilesAudioEngine( [[maybe_unused]] void *pEngine )
{
	// This function is never called because CreateMilesAudioEngine returns nullptr
}
#endif


CVAudioMiniMP3 &CVAudioMiniMP3::GetInstance()
{
	// We must allocate this as some Source Engine versions attempt
	// to delete the vaudio pointer on shutdown.
	static CVAudioMiniMP3 *s_pVAudio = new CVAudioMiniMP3;
	return *s_pVAudio;
}


//-----------------------------------------------------------------------------
// Interface
//-----------------------------------------------------------------------------

// In Desolation, we build all vaudio components inside of the engine.
#ifdef ENGINE_DLL
IVAudio *g_pVAudio = &CVAudioMiniMP3::GetInstance();
#else
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVAudioMiniMP3, IVAudio, VAUDIO_INTERFACE_VERSION, CVAudioMiniMP3::GetInstance() );
#endif
