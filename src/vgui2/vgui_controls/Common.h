#pragma once

// Used for unity build

namespace vgui
{

enum 
{
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

static bool BindingLessFunc( KeyValues * const & lhs, KeyValues * const &rhs )
{
	KeyValues *p1, *p2;

	p1 = const_cast< KeyValues * >( lhs );
	p2 = const_cast< KeyValues * >( rhs );
	return ( Q_stricmp( p1->GetString( "Action" ), p2->GetString( "Action" ) ) < 0 ) ? true : false;
}

static char *CopyString( const char *in )
{
	if ( !in )
		return NULL;

	int len = V_strlen( in );
	char *n = new char[ len + 1 ];
	V_strncpy( n, in, len  + 1 );
	return n;
}

static int ListFileNameSortFunc([[maybe_unused]] ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if ( dir1 != dir2 )
	{
		return dir1 ? -1 : 1;
	}

	const char *string1 = item1.kv->GetString("text");
	const char *string2 = item2.kv->GetString("text");

	// YWB:  Mimic windows behavior where filenames starting with numbers are sorted based on numeric part
	int num1 = Q_atoi( string1 );
	int num2 = Q_atoi( string2 );

	if ( num1 != 0 && 
		 num2 != 0 )
	{
		if ( num1 < num2 )
			return -1;
		else if ( num1 > num2 )
			return 1;
	}

	// Push numbers before everything else
	if ( num1 != 0 )
	{
		return -1;
	}
	
	// Push numbers before everything else
	if ( num2 != 0 )
	{
		return 1;
	}

	return Q_stricmp( string1, string2 );
}

static int ListBaseStringSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2, char const *fieldName )
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	const char *string1 = item1.kv->GetString(fieldName);
	const char *string2 = item2.kv->GetString(fieldName);
	int cval = Q_stricmp(string1, string2);
	if ( cval == 0 )
	{
		// Use filename to break ties
		return ListFileNameSortFunc( pPanel, item1, item2 );
	}

	return cval;
}

static int ListBaseIntegerSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2, char const *fieldName )
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	int i1 = item1.kv->GetInt(fieldName);
	int i2 = item2.kv->GetInt(fieldName);
	if ( i1 == i2 )
	{
		// Use filename to break ties
		return ListFileNameSortFunc( pPanel, item1, item2 );
	}

	return ( i1 < i2 ) ? -1 : 1;
}

static int ListFileSizeSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseIntegerSortFunc( pPanel, item1, item2, "filesizeint" );
}

static int ListFileAttributesSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseStringSortFunc( pPanel, item1, item2, "attributes" );
}

static int ListFileTypeSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseStringSortFunc( pPanel, item1, item2, "type" );
}

struct ColumnInfo_t
{
	char const	*columnName;
	char const	*columnText;
	int			startingWidth;
	int			minWidth;
	int			maxWidth;
	int			flags;
	SortFunc	*pfnSort;
	Label::Alignment alignment;
};

} // namespace vgui

