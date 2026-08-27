///////////////////////////////////////////////
// A non-networked non-entity PointWorldText //
///////////////////////////////////////////////

class PointWorldText
{
public:
	PointWorldText();
	PointWorldText(const char* pszText, Vector pos, CMaterialReference* font);
	~PointWorldText();

	int DrawModel(float alpha = 1.0f);

	void SetText(const char* pszText);

	Vector GetAbsOrigin() { return m_vecAbsOrigin; }
	QAngle GetAbsAngles() { return m_vecAbsAngles; }

private:
	void CalcTextTotalSize(float &outWidth, float &outHeight);
	void UpdateTextWorldSize();

	float GetTextWorldWidth() const;
	float GetTextWorldHeight() const;
	float GetTextSpacingX() const;
	float GetTextSpacingY() const;

	Vector m_vecAbsOrigin = {0, 0, 0};
	QAngle m_vecAbsAngles = {0, 0, 0};

	char m_szText[ MAX_PLACE_NAME_LENGTH ];
	float m_flTextSize = 64.f;
	float m_flTextSpacingX = 0.f;
	float m_flTextSpacingY = 0.f;
	color32 m_colTextColor = {255, 255, 255, 255};
	int m_nOrientation = 3;
	int m_nTextLength = 0;

	float m_flTextWorldWidth = 0.f;
	float m_flTextWorldHeight = 0.f;
	
	CMaterialReference* m_Font;
};