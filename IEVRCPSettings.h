#pragma once

enum EVRCPSetting
{
  EVRCP_SETTING_NOMINAL_RANGE = 0,
  EVRCP_SETTING_SUBTITLE_ALPHA,
  EVRCP_SETTING_USE_MF_TIME_CALC,
  EVRCP_SETTING_FRAME_DROP_THRESHOLD,
  EVRCP_SETTING_CORRECT_AR
};

[uuid("D54059EF-CA38-46A5-9123-0249770482EE")]
interface IEVRCPSettings : public IUnknown
{
	STDMETHOD(SetNominalRange)(MFNominalRange dwNum) = 0;
	STDMETHOD_(MFNominalRange,GetNominalRange)() = 0;
	STDMETHOD(SetSubtitleAlpha)(float fAlpha) = 0;
	STDMETHOD_(float,GetSubtitleAlpha)() = 0;
};

[uuid("759B5834-8F83-4855-9484-DC183B381D84")]
interface IEVRCPConfig : public IUnknown
{
	STDMETHOD(GetInt)(EVRCPSetting setting, int* value) = 0;
	STDMETHOD(SetInt)(EVRCPSetting setting, int value) = 0;
	STDMETHOD(GetFloat)(EVRCPSetting setting, float* value) = 0;	
	STDMETHOD(SetFloat)(EVRCPSetting setting, float value) = 0;
	STDMETHOD(GetBool)(EVRCPSetting setting, bool* value) = 0;
	STDMETHOD(SetBool)(EVRCPSetting setting, bool value) = 0;
	STDMETHOD(GetString)(EVRCPSetting setting, LPWSTR value) = 0;
	STDMETHOD(SetString)(EVRCPSetting setting, LPWSTR value) = 0;
};