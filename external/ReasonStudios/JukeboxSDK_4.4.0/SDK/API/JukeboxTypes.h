/*  Copyright (c) 2011 Reason Studios AB. All rights reserved. */

#pragma once

#ifndef PROPELLERHEAD_JUKEBOXTYPES_H
#define PROPELLERHEAD_JUKEBOXTYPES_H

#include <cstddef>

/**
	@file
	@brief Jukebox API types and enums
*/

/*
	Platform abstractions for LLVM and native targets
	JBOX_IS_RE				Rack Extension code, same as __phdsp__
	JBOX_IS_HOST_MAC		Host code on Mac
	JBOX_IS_HOST_WINDOWS	Host code on Windows
	JBOX_IS_32				32 bit
	JBOX_IS_64				64 bit
*/


#if __phdsp__
	#define JBOX_IS_RE 1
	#define JBOX_IS_HOST_MAC 0
	#define JBOX_IS_HOST_WINDOWS 0
	#if __phdsp64__
	#define JBOX_IS_32 0
	#define JBOX_IS_64 1
	#elif __phdsp32__
	#define JBOX_IS_32 1
	#define JBOX_IS_64 0
	#else
	#error "Undefined __phdsp__ sub-target!"
	#endif

	typedef float TJBox_Float32;
	typedef double TJBox_Float64;

	typedef unsigned char TJBox_Bool;	/*	TRUE or FALSE */

	typedef signed char TJBox_Int8;
	typedef unsigned char TJBox_UInt8;

	typedef signed short TJBox_Int16;
	typedef unsigned short TJBox_UInt16;

	typedef signed int TJBox_Int32;
	typedef unsigned int TJBox_UInt32;

	#if JBOX_IS_32
	typedef signed long long TJBox_Int64;
	typedef unsigned long long TJBox_UInt64;
	#elif JBOX_IS_64
	typedef signed long TJBox_Int64;
	typedef unsigned long TJBox_UInt64;
	#endif

	typedef unsigned long TJBox_SizeT;
	typedef unsigned long TJBox_PtrSize;
	typedef signed long TJBox_PtrDiff;

#elif __GNUC__ /* __phdsp__ */
	#define JBOX_IS_RE 0
	#define JBOX_IS_HOST_MAC 1
	#define JBOX_IS_HOST_WINDOWS 0
	#if __LP64__
	#define JBOX_IS_32 0
	#define JBOX_IS_64 1
	#else
	#define JBOX_IS_32 1
	#define JBOX_IS_64 0
	#endif
	
	typedef float TJBox_Float32;
	typedef double TJBox_Float64;

	typedef unsigned char TJBox_Bool;	/*	TRUE or FALSE */
	
	typedef signed char TJBox_Int8;
	typedef unsigned char TJBox_UInt8;
	
	typedef signed short TJBox_Int16;
	typedef unsigned short TJBox_UInt16;
	
	/*
		RP: The below works for __LP64__ to, although long and long long are the same size
		they are not the same type and long long is compatible with std::int64_t
		### FL: I don't know if this comment was related to TJBox_Int64's or also the TJBox_SizeT
	*/
	
	typedef int TJBox_Int32;
	typedef unsigned int TJBox_UInt32;
	
	typedef long long TJBox_Int64;
	typedef unsigned long long TJBox_UInt64;	

	typedef unsigned long TJBox_SizeT;
	typedef unsigned long TJBox_PtrSize;
	typedef long TJBox_PtrDiff;

#elif _MSC_VER
	#define JBOX_IS_RE 0
	#define JBOX_IS_HOST_MAC 0
	#define JBOX_IS_HOST_WINDOWS 1
	#if _M_X64
	#define JBOX_IS_32 0
	#define JBOX_IS_64 1
	#else
	#define JBOX_IS_32 1
	#define JBOX_IS_64 0
	#endif

	typedef float TJBox_Float32;
	typedef double TJBox_Float64;

	typedef unsigned char TJBox_Bool;	/*	TRUE or FALSE */

	typedef signed char TJBox_Int8;
	typedef unsigned char TJBox_UInt8;

	typedef signed short TJBox_Int16;
	typedef unsigned short TJBox_UInt16;

	/* RP: To match the definition of std::int32_t and std::uint32_t */
	typedef long TJBox_Int32;
	typedef unsigned long TJBox_UInt32;

	typedef __int64 TJBox_Int64;
	typedef unsigned __int64 TJBox_UInt64;

	#if JBOX_IS_64
	typedef TJBox_UInt64 TJBox_SizeT;
	typedef TJBox_UInt64 TJBox_PtrSize;
	typedef TJBox_Int64 TJBox_PtrDiff;
	#else
	typedef TJBox_UInt32 TJBox_SizeT;
	typedef TJBox_UInt32 TJBox_PtrSize;
	typedef TJBox_Int64 TJBox_PtrDiff;
	#endif

#else
#error "Unknown compiler."
#endif /* !__phdsp__ */



/**
	@brief Type that specifies a position within an audio buffer.
	@details
		An audio frame consists of one sample for each channel in the
		buffer.
*/
typedef TJBox_Int32 TJBox_AudioFramePos;

/**
	@brief Type that represents a single-channel audio sample.
*/
typedef TJBox_Float32 TJBox_AudioSample;


/* //////////////////		Values
*/

/**
	@brief The maximum length of property names in Jukebox, not including
		the terminating zero.
*/
#define kJBox_MaxPropertyNameLen	35
/**
	@brief Type that represents a property key.
*/
typedef char TJBox_PropertyKey[kJBox_MaxPropertyNameLen + 1];

/**
	@brief The maximum length of Motherboard property set object names in Jukebox, 
		not including the terminating zero.
*/
#define kJBox_MaxObjectNameLen		63

/**
 @brief A value used by /transport/pattern_index to indicate that no pattern is selected.
 */
#define kJBox_NoPatternIndex -1

/**
	@brief Type that represents a Motherboard property set object name.
*/
typedef char TJBox_ObjectName[kJBox_MaxObjectNameLen + 1];

/**
	@brief Dynamic value types.
*/
typedef enum {
	kJBox_Nil = 2,
	kJBox_Number,
	kJBox_String,
	kJBox_Boolean,
	kJBox_Sample,
	kJBox_BLOB,
	kJBox_DSPBuffer,
	kJBox_NativeObject,
	kJBox_Incompatible
} TJBox_ValueType;

/**
 	@brief States for the "custom_properties/builtin_onoffbypass" property.
 */
typedef enum {
	kJBox_EnabledOff = 0,
	kJBox_EnabledOn,
	kJBox_EnabledBypass
} TJBox_OnOffBypassStates;

/**
 	@brief Tag for accessing the "custom_properties/builtin_onoffbypass" 
 		property.
 */
typedef enum {
	kJBox_CustomPropertiesOnOffBypass = 0xFFFFFFFD
} TJBox_CustomPropertyTag;

/**
	@brief Tags for accessing properties in the "transport" 
		Motherboard object.
*/
typedef enum {
	kJBox_TransportPlaying = 23,
	kJBox_TransportPlayPos,
	kJBox_TransportTempo,
	kJBox_TransportFilteredTempo,
	kJBox_TransportTempoAutomation,
	kJBox_TransportTimeSignatureNumerator,
	kJBox_TransportTimeSignatureDenominator,
	kJBox_TransportRequestResetAudio,
	kJBox_TransportLoopEnabled,
	kJBox_TransportLoopStartPos,
	kJBox_TransportLoopEndPos,
	kJBox_TransportBarStartPos,
	kJBox_TransportRequestRun,
	kJBox_TransportRequestStop,
	kJBox_TransportMuted,
	kJBox_TransportPatternStartPos,
	kJBox_TransportPatternIndex
} TJBox_TransportTag;

/**
	@brief Tags for accessing properties in the "environment" 
		Motherboard object.
 */
typedef enum {
	kJBox_EnvironmentMasterTune = 43,
	kJBox_EnvironmentSystemSampleRate,
	kJBox_EnvironmentInstanceID,
	kJBox_EnvironmentDeviceVisible,
	kJBox_EnvironmentPlayerBypassed
} TJBox_EnvironmentTag;

/**
	@brief Tags for accessing properties in the "audio_input" 
		Motherboard object.
 */
typedef enum {
	kJBox_AudioInputBuffer = 73,
	kJBox_AudioInputConnected
} TJBox_AudioInputTag;

/**
	@brief Tags for accessing properties in the "audio_output" 
		Motherboard object.
 */
typedef enum {
	kJBox_AudioOutputBuffer = 83,
	kJBox_AudioOutputConnected,
	kJBox_AudioOutputDSPLatency
} TJBox_AudioOutputTag;

/**
	@brief Tags for accessing properties in the "cv_input" 
		Motherboard object.
 */
typedef enum {
	kJBox_CVInputValue = 93,
	kJBox_CVInputConnected = 95
} TJBox_CVInputTag;

/**
	@brief Tags for accessing properties in the "cv_output" 
		Motherboard object.
 */
typedef enum {
	kJBox_CVOutputValue = 103,
	kJBox_CVOutputConnected,
	kJBox_CVOutputDSPLatency
} TJBox_CVOutputTag;

/**
	@brief Tags for accessing properties in "user_sample" Motherboard objects.
 */
typedef enum {
	kJBox_UserSampleRootKey = 0,
	kJBox_UserSampleTuneCents,
	kJBox_UserSamplePlayRangeStart,
	kJBox_UserSamplePlayRangeEnd,
	kJBox_UserSampleLoopRangeStart,
	kJBox_UserSampleLoopRangeEnd,
	kJBox_UserSampleLoopMode,
	kJBox_UserSamplePreviewVolumeLevel,
	kJBox_UserSampleItem
} TJBox_UserSampleTag;

/**
	@brief Tags for accessing properties in "device_host" Motherboard objects.
 */
typedef enum {
	kJBox_DeviceHostSampleContext = 0,
	kJBox_DeviceHostDeleteSample,
	kJBox_DeviceHostEditSample
} TJBox_DeviceHostTag;

/**
 @brief Tags for accessing properties in "patterns" Motherboard objects.
 */
typedef enum {
	kJBox_PatternsLength = 0,
} TJBox_PatternsTag;

/**
	@brief Information about BLOB dynamic values.
*/
typedef struct {
    /** @brief The total size of the BLOB in bytes. */
	TJBox_SizeT fSize;
    /** @brief The number of bytes of the BLOB that are currently loaded in memory. */
	TJBox_SizeT fResidentSize;
} TJBox_BLOBInfo;

#if JBOX_IS_64
static_assert(offsetof(TJBox_BLOBInfo, fSize) == 0);
static_assert(offsetof(TJBox_BLOBInfo, fResidentSize) == 8);
static_assert(sizeof(TJBox_BLOBInfo) == 16);
#else
static_assert(offsetof(TJBox_BLOBInfo, fSize) == 0);
static_assert(offsetof(TJBox_BLOBInfo, fResidentSize) == 4);
static_assert(sizeof(TJBox_BLOBInfo) == 8);
#endif

/**
	@brief Information about sample dynamic values.
*/
typedef struct {
    /** @brief The total number of frames in the sample. */
	TJBox_Int64 fFrameCount;
    /** @brief The number of frames of the sample that are currently loaded in memory. */
	TJBox_Int64 fResidentFrameCount;
    /** @brief The number of channels in the sample, e.g., 2 for stereo. */
	TJBox_UInt32 fChannels;
    /** @brief The sample rate of the sample in Hz. */
	TJBox_UInt32 fSampleRate;
} TJBox_SampleInfo;

static_assert(offsetof(TJBox_SampleInfo, fFrameCount) == 0);
static_assert(offsetof(TJBox_SampleInfo, fResidentFrameCount) == 8);
static_assert(offsetof(TJBox_SampleInfo, fChannels) == 16);
static_assert(offsetof(TJBox_SampleInfo, fSampleRate) == 20);
static_assert(sizeof(TJBox_SampleInfo) == 24);


typedef struct {
	/** @brief The total number of frames in the sample. */
	TJBox_AudioFramePos fFrameCount;
	/** @brief The number of channels in the sample, e.g., 2 for stereo. */
	TJBox_UInt32 fChannels;
	/** @brief The sample rate of the sample in Hz. */
	TJBox_UInt32 fSampleRate;
} TJBox_SampleSpec;

static_assert(offsetof(TJBox_SampleSpec, fFrameCount) == 0);
static_assert(offsetof(TJBox_SampleSpec, fChannels) == 4);
static_assert(offsetof(TJBox_SampleSpec, fSampleRate) == 8);
static_assert(sizeof(TJBox_SampleSpec) == 12);


/** @brief The samples parameters in Reason. These may or may not be
	the same as those of the user_sample's parameters in the motherboard */
typedef struct {
	TJBox_UInt8 fRootNote;
	TJBox_Int8 fTuneCents;
	TJBox_AudioFramePos fPlayRangeStart;
	TJBox_AudioFramePos fPlayRangeEnd;
	TJBox_AudioFramePos fLoopRangeStart;
	TJBox_AudioFramePos fLoopRangeEnd;
	TJBox_UInt8 fLoopMode;
	TJBox_UInt8 fVolumeLevel;
} TJBox_SampleParameters;

static_assert(offsetof(TJBox_SampleParameters, fRootNote) == 0);
static_assert(offsetof(TJBox_SampleParameters, fTuneCents) == 1);
static_assert(offsetof(TJBox_SampleParameters, fPlayRangeStart) == 4);
static_assert(offsetof(TJBox_SampleParameters, fPlayRangeEnd) == 8);
static_assert(offsetof(TJBox_SampleParameters, fLoopRangeStart) == 12);
static_assert(offsetof(TJBox_SampleParameters, fLoopRangeEnd) == 16);
static_assert(offsetof(TJBox_SampleParameters, fLoopMode) == 20);
static_assert(offsetof(TJBox_SampleParameters, fVolumeLevel) == 21);
static_assert(sizeof(TJBox_SampleParameters) == 24);


/** @brief Returned by JBox_GetSampleMetaData */
typedef struct {
	TJBox_SampleSpec fSpec;
	TJBox_SampleParameters fParameters;
	TJBox_AudioFramePos fResidentFrameCount;
	/** @brief The sample's load status. May be:
		0 - Empty sample, has read errors, missing sample
		1 - Loading in progress
		2 - Resident
		Only call Jbox_GetSampleData for resident samples */
	TJBox_UInt8 fStatus;
} TJBox_SampleMetaData;

static_assert(offsetof(TJBox_SampleMetaData, fSpec) == 0);
static_assert(offsetof(TJBox_SampleMetaData, fParameters) == 12);
static_assert(offsetof(TJBox_SampleMetaData, fResidentFrameCount) == 36);
static_assert(offsetof(TJBox_SampleMetaData, fStatus) == 40);
static_assert(sizeof(TJBox_SampleMetaData) == 44);




/**
	@brief Information about DSP buffer dynamic values.
*/
typedef struct {
    /** @brief The total number of samples in the DSP buffer. */
	TJBox_Int64 fSampleCount;
} TJBox_DSPBufferInfo;

static_assert(offsetof(TJBox_DSPBufferInfo, fSampleCount) == 0);
static_assert(sizeof(TJBox_DSPBufferInfo) == 8);


/**
	@brief Motherboard property set object reference.
*/
typedef TJBox_Int32 TJBox_ObjectRef;

/**
	@brief Property reference.
*/
typedef struct {
	/** @brief Property set object reference to the owner of this property. */ 
	TJBox_ObjectRef fObject;
	/**
		@brief Name of the property in the object as a zero-terminated ASCII string.
		@details Max length of the property name is kJBox_MaxPropertyNameLen, 
			not including the terminating zero.
	*/
	TJBox_PropertyKey fKey;
} TJBox_PropertyRef;

static_assert(offsetof(TJBox_PropertyRef, fObject) == 0);
static_assert(offsetof(TJBox_PropertyRef, fKey) == 4);
static_assert(sizeof(TJBox_PropertyRef) == 40);

/**
	@brief Property tag.
*/
typedef TJBox_UInt32 TJBox_Tag;
/**
	@brief Invalid property tag.
*/
#define kJBox_InvalidPropertyTag ((TJBox_Tag)(-1))

/**
	@brief Opaque data type that holds a dynamic value.
	@details
		Dynamic values cannot be stored in C++ objects.
*/
typedef struct {
	TJBox_UInt8 fSecret[16];
} TJBox_Value;

static_assert(offsetof(TJBox_Value, fSecret) == 0);
static_assert(sizeof(TJBox_Value) == 16);

/**
	@brief Represents a property change.
	@details
		Contains information about when the property was changed
		and its previous and current value.
*/
typedef struct {
	/** @brief The value of the property before the change. */
	TJBox_Value fPreviousValue;
	/** @brief The current value of the property. */
	TJBox_Value fCurrentValue;
	/** @brief Property reference. */
	TJBox_PropertyRef fPropertyRef;
	/** @brief The property tag if one has been assigned, 
		kJBox_InvalidPropertyTag otherwise. */
	TJBox_Tag fPropertyTag;
	/**
		@brief The frame in the batch where the change took place.
	    	This value will normally be between 0 and 63 but can, 
	    	because of a bug in Reason 6.5, be higher,
			in which case it can be safely treated as 63.
	*/
	TJBox_UInt16 fAtFrameIndex;
} TJBox_PropertyDiff;

static_assert(offsetof(TJBox_PropertyDiff, fPreviousValue) == 0);
static_assert(offsetof(TJBox_PropertyDiff, fCurrentValue) == 16);
static_assert(offsetof(TJBox_PropertyDiff, fPropertyRef) == 32);
static_assert(offsetof(TJBox_PropertyDiff, fPropertyTag) == 72);
static_assert(offsetof(TJBox_PropertyDiff, fAtFrameIndex) == 76);
static_assert(sizeof(TJBox_PropertyDiff) == 80);

/**
    @brief Represents a note event.
    @details
        Input note events that are received as property diffs can be converted
        to this type. This is also the data type that are used when outputting
        note events to the host.
		A note event contains information about when the event happened (frame index),
        its note number, and its velocity.
        Events with a zero velocity turns off a note.
        Events with a velocity greater than zero turns on a note.
*/
typedef struct {
    /** @brief The note number of the event, valid values are between 0 and 127.  */
    TJBox_UInt8 fNoteNumber;
    /** @brief The velocity of the event, valid values are between 0 and 127. 0 velocity means that the note is turned off.*/
    TJBox_UInt8 fVelocity;
    /**
        @brief The frame in the batch where the event took place.
            This value must be between 0 and 63.
    */
    TJBox_UInt16 fAtFrameIndex;
} TJBox_NoteEvent;

static_assert(offsetof(TJBox_NoteEvent, fNoteNumber) == 0);
static_assert(offsetof(TJBox_NoteEvent, fVelocity) == 1);
static_assert(offsetof(TJBox_NoteEvent, fAtFrameIndex) == 2);
static_assert(sizeof(TJBox_NoteEvent) == 4);

#endif /* PROPELLERHEAD_JUKEBOXTYPES_H */

