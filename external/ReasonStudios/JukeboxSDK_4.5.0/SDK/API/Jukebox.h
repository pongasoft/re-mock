/*  Copyright (c) 2011 - 2013 Reason Studios AB. All rights reserved. */

#pragma once

#ifndef PROPELLERHEAD_JUKEBOX_H
#define PROPELLERHEAD_JUKEBOX_H

/**
	@file
	@brief The interface between the C++ code of a Jukebox 45 and the host.
	@details
		This header defines the toolbox available for the C++ part of a
		Jukebox 45,and the functions that every 45 must implement and 
		export.

		Except for a limited set of standard library functions, this is
		the complete external support that a Jukebox 45 will get. There is 
		no direct access to OS libraries, stdio or the like.

		The API is limited to C for maximum portability and longevity,
		but 45 code is compiled as C++.

		The API is very strict and error handling is limited. All 
		programming errors will lead to immediate termination of the 45 
		instance. It is possible to compile code with exception handling, 
		but a thrown exception is considered a programming error.
*/

#include "JukeboxTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
	@brief If the expression given to this macro compares equal to zero 
		(i.e., the expression is false), a message to this effect is 
		presented and the 45 is disabled.
	@details This macro only has effect in LLVM and Debugging builds of
		the 45 (not in Deployment).
	@param expression The expression to be evaluated; is shown in the 
		message.
*/
#if DEBUG
#define JBOX_ASSERT(expression) \
    if (expression) {} else { JBox_Assert(__FILE__, __LINE__, #expression, ""); }
#else
#define JBOX_ASSERT(expression)
#endif

/**
	@brief If the expression given to this macro compares equal to zero 
		(i.e., the expression is false), a message to this effect is 
		presented and the 45 is disabled.
	@details This macro only has effect in LLVM and Debugging builds of
		the 45 (not in Deployment).
	@param[in] expression The expression to be evaluated; is shown in the 
		message.
	@param[in] message A null-terminated text string that can be used to 
		provide additional information to the user. 
*/
#if DEBUG
#define JBOX_ASSERT_MESSAGE(expression, message) \
if (expression) {} else { JBox_Assert(__FILE__, __LINE__, #expression, message); }
#else
#define JBOX_ASSERT_MESSAGE(expression, message)
#endif
	
/**
	@brief Utility function that calls JBox_Trace with the specified
		message.
	@param[in] message The message to trace.
	@see JBox_Trace()
*/
#if DEBUG
#define JBOX_TRACE(message) \
    JBox_Trace(__FILE__, __LINE__, message)
#else
#define JBOX_TRACE(message)
#endif

/**
	@brief Utility function that calls JBox_TraceValues with the 
		specified input.
	@param[in] template Message to be printed. Zero-terminated ASCII
		or UTF-8. The substring 
		@code 
		^0
		@endcode
		will be replaced with iValues[0], the substring 
		@code 
		^1 
		@endcode
		will be replaced with iValues[1], etc.
	@param[in] values Array of values to be traced.
	@param[in] valueCount Size of the value array.
	@see JBox_TraceValues()
*/
#if DEBUG
#define JBOX_TRACEVALUES(template, values, valueCount) \
	JBox_TraceValues(__FILE__, __LINE__, template, values, valueCount)
#else
#define JBOX_TRACEVALUES(template, values, valueCount)
#endif

/**
	@defgroup jukebox_45_exports Jukebox 45 Exports
	@brief This module contains functions that all 45s must implement.
	@details
		All processing made by a 45 is done in these functions, either in a
		real time synchronous or idle time asynchronous manner.
*/

/**
	@defgroup jukebox_mom_functions Functions for accessing the MOM
	@brief These functions are used to access the MOM. They can only 
		be called from JBox_Export_RenderRealtime.
	@details
		Inspecting or updating the motherboard object model is only
		allowed from the context of JBox_Export_RenderRealtime.
		(JBox_Export_CreateNativeObject is called asynchronously and 
		can only perform pure functional tasks based on its inputs.)
		Calling these functions from the wrong context will disable
		the 45 instance.
*/

/**
	@brief Performs realtime audio processing for a 45.
	@details
		Processes realtime audio streams and reacts to motherboard
		changes.

		JBox_Export_RenderRealtime is usually called in a realtime
		context, and must perform its work in as short time as possible.
		(The total time available for audio processing for the entire
		rack is about 1.5 milliseconds per batch.)
		
		Only a subset of the functions in the Jukebox Toolbox can be 
		called from JBox_Export_RenderRealtime. Calling illegal 
		functions will abort the current JBox_Export_RenderRealtime 
		call and disable the 45 instance.
	
		It is not possible to do memory allocations or time-consuming
		calculations from JBox_Export_RenderRealtime; use 
		JBox_Export_CreateNativeObject for such tasks.

		A list of subscribed properties that have changed since the last
		call to JBox_Export_RenderRealtime is given as input. Enabling
		change notification for properties is optional, since it is always
		possible to inspect the MOM directly from JBox_Export_RenderRealtime.
		However, without notifications it is possible to miss multiple 
		property changes inside a single batch (e.g., a gate property 
		switching between off and on several times during a batch).

		The differences in the list are sorted by their respective frame
		position within the current batch. The changes have already been 
		applied to the MOM when JBox_Export_RenderRealtime is called.
		
		If an audio output is not written to during a call to 
		JBox_Export_RenderRealtime, that audio output will be considered
		silent for the current batch. This information is used by the host
		to optimize the audio rendering chain. If an audio output is written
		to, all the values in the DSP buffer must be updated. Storing
		nan:s or inf:s in a DSP buffer is not allowed.
		
		If a CV output is not written to during a call to
		JBox_Export_RenderRealtime, that output will keep its current 
		value, just like custom properties.

	@param[in] iInstanceData Current value of the "environment/instance" 
		property, for convenience. The value of this property normally
		represents a native object that holds the C++ state for the 45
		instance. The native object is normally created with a call to
		make_native_object when the 45 instance is created.
	@param[in] iPropertyDiffs List of properties that have changed since the
		last call to JBox_Export_RenderRealtime.
	@param[in] iDiffCount The size of the iPropertyDiffs array.
	@ingroup jukebox_45_exports
*/
void JBox_Export_RenderRealtime(
	void* iInstanceData, 
	const TJBox_PropertyDiff iPropertyDiffs[], 
	TJBox_UInt32 iDiffCount);

/**
	@brief Generic constructor for creating C++ objects.
	@details
		This function is used to create or process data that requires 
		complex calculations, memory allocation, or the use of Jukebox
		Toolbox functions that are unavailable in 
		JBox_Export_RenderRealtime.
 
		JBox_Export_CreateNativeObject is invoked via the realtime 
		controller Lua script, either as an effect of a property change, 
		or during 45 instance initialization to create the instance data
		that is passed in each call to JBox_Export_RenderRealtime.
		
		The combination of iOperation and iParams that configure the 
		result of JBox_Export_CreateNativeObject is up to each 45 
		developer to define.

		To get access to the native object created by JBox_Export_CreateNativeObject
		from JBox_Export_RenderRealtime, the object must be assigned
		to the motherboard via the realtime controller Lua script.

		Native objects are ordinary C/C++ data structures and may contain
		pointers and references. During JBox_Export_CreateNativeObject you 
		can allocate memory with malloc, new, stl containers etc. All these
		allocations are stored inside the native object and they can refer
		to each other. There must be no pointers pointing outside of the native 
		object's allocations.

		Instances of TJBox_Value that are obtained and used inside
		a call to JBox_Export_CreateNativeObject are only valid during 
		call. Is is not possible to store any such value in the native object.
	
		All memory allocations are noted by the host so if the host must
		disable a 45, no resources will leak.

	@param[in] iOperation String key used by the realtime controller Lua 
		script to identify which object to create.
	@param[in] iParams Array of parameter values for the operation
		identified by iOperation.
	@param[in] iParamCount The size of the iParams array.
	@return Pointer to the new object.
	@ingroup jukebox_45_exports
*/
void* JBox_Export_CreateNativeObject(
	const char iOperation[], 
	const TJBox_Value iParams[], 
	TJBox_UInt32 iParamCount);


/* Toolbox */

/**
	@brief Notifies the Jukebox host about a failed assert.
	@details
		Calling JBox_Assert in Debug builds will cause the host to stop
		45 execution and notify the user. The host might terminate the 
		45 or continue execution depending on the implementation.
		Calls to JBox_Assert in Deployoment builds will terminate the
		45 instance without notifying the user.
	@param[in] iFile Name of a source code file from where the message 
		originates. Zero-terminated ASCII or UTF-8.
	@param[in] iLine Line number in the source file where the message 
		originates.
	@param[in] iFailedExpression String that represents the expression that
		failed. Zero-terminated ASCII or UTF-8.
	@param[in] iMessage String containing the message to present to
		the user. Zero-terminated ASCII or UTF-8.
*/
void JBox_Assert(
	const char iFile[], 
	TJBox_Int32 iLine,
	const char iFailedExpression[],
	const char iMessage[]);

/**
	@brief Requests that the Jukebox host traces a message.
	@details
		The trace message might be written to a console or to a file,
		depending on the host. The host is free to reformat the message
		as needed, adding timestamps, etc.
		A newline is added to each message before it is traced.
	@param[in] iFile Name of a source code file from where the message 
		originates. Zero-terminated ASCII or UTF-8.
	@param[in] iLine Line number in the source file where the message 
		originates.
	@param[in] iMessage String containing the message to present to
		the user. Zero-terminated ASCII or UTF-8.
*/
void JBox_Trace(
	const char iFile[], 
	TJBox_Int32 iLine, 
	const char iMessage[]);

/**
	@brief Requests that the Jukebox host traces a formatted message
		containing TJbox_Value:s.
	@details
		Similar to JBox_Trace(), but with simple message formatting
		capabilities.
	@param[in] iFile Name of a source code file from where the message
		originates. Zero-terminated ASCII or UTF-8.
	@param[in] iLine Line number in the source file where the message 
		originates. Zero-terminated ASCII or UTF-8.
	@param[in] iTemplate Message to be printed. Zero-terminated ASCII
		or UTF-8. The substring 
		@code 
		^0
		@endcode
		will be replaced with iValues[0], the substring 
		@code 
		^1 
		@endcode
		will be replaced with iValues[1], etc.
	@param[in] iValues Array of values to be traced.
	@param[in] iValueCount Size of the value array.
*/
void JBox_TraceValues(
	const char iFile[], 
	TJBox_Int32 iLine, 
	const char iTemplate[], 
	const TJBox_Value iValues[], 
	TJBox_Int32 iValueCount);


/* Functions for accessing the MOM. */

/** 
	@brief Resolves a motherboard property set object path.
	@details
		This function is used to obtain a reference to any motherboard 
		property set object	(built-in or custom). Object references are valid
		during the whole lifetime of a 45 instance. Trying to resolve an 
		undefined property by specifying an invalid path will blow the fuse.
	@param[in] iMOMPath
		Path identifier for the motherboard property set object, e.g., 
		"custom_properties", "note_states", "audio_inputs", "cv_outputs", etc.
	@return Reference to the specified motherboard property set object.
	@see JBox_MakePropertyRef()
	@ingroup jukebox_mom_functions
*/
TJBox_ObjectRef JBox_GetMotherboardObjectRef(const TJBox_ObjectName iMOMPath);

/**
	@brief Obtain a property reference.
	@details
		Property references are valid during the whole lifetime of a 45
		instance. Specifying an invalid property set object reference or 
		unknown property key will blow the fuse.
	@param[in] iObject Reference to the Motherboard property set object 
		that holds the requested property.
	@param[in] iKey Name of the property.
	@return Reference to the specified property.
	@see JBox_GetMotherboardObjectRef()
	@ingroup jukebox_mom_functions
*/
TJBox_PropertyRef JBox_MakePropertyRef(
	TJBox_ObjectRef iObject,
	const TJBox_PropertyKey iKey);

/**
	@brief Test two property references for equality.
	@details
		Two property references are equal if they point to the same 
		property key and are inside the same Motherboard property set object.
	@param[in] iProperty1 First property to be compared.
	@param[in] iProperty2 Second property to be compared.
	@return True if iProperty1 and iProperty2 represents the same
		property, false otherwise.
*/
TJBox_Bool JBox_IsReferencingSameProperty(
	TJBox_PropertyRef iProperty1, 
	TJBox_PropertyRef iProperty2);



/**
	@brief Retrieves the user-defined (or system-defined) tag for
		a property.
	@details
		Numerical tags can be assigned to Motherboard properties
		to facilitate switch statements or looping over ranges of
		properties.

		All property tags inside a specific Motherboard property set 
		object must be unique. The same tag may be reused in different 
		Motherboard property set objects.

		Requesting an invalid tag will terminate the 45 instance.
	@param[in] iProperty The property from which the tag is
		obtained.
	@return The tag for the specified property.
	@ingroup jukebox_mom_functions
*/
TJBox_Tag JBox_GetPropertyTag(TJBox_PropertyRef iProperty);

/**
	@brief Retrieves a property via its numerical tag.
	@details
		Requesting an invalid tag will terminate the 45 instance.
	@param[in] iObject Reference to the Motherboard property set 
		object that holds the requested property.
	@param[in] iTag The tag of the property to retrieve.
	@return The specified property.
	@ingroup jukebox_mom_functions
*/
TJBox_PropertyRef JBox_FindPropertyByTag(
	TJBox_ObjectRef iObject, 
	TJBox_Tag iTag);


/* Functions for loading / storing property values. */

/**
	@brief Retrieve the value of a property.
	@details
		Requesting the value of an invalid property will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iProperty The property to retrieve.
	@return The value of the property.
	@ingroup jukebox_mom_functions
*/
TJBox_Value JBox_LoadMOMProperty(TJBox_PropertyRef iProperty);

/**
	@brief Update the value of a property.
	@details
		Only properties in the rt_owner scope can be updated.
		The updated value can be read back directly (using, e.g.,
		JBox_LoadMOMProperty), but it will not be propagated
		to the Motherboard until JBox_Export_RenderRealtime 
		returns.
		Updating the value of an invalid property will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iProperty The property to update.
	@param[in] iValue The new value.
	@ingroup jukebox_mom_functions
*/
void JBox_StoreMOMProperty(TJBox_PropertyRef iProperty, TJBox_Value iValue);

/**
	@brief Retrieve the value of a property.
	@details
		Requesting the value of an invalid property will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iObject The Motherboard property set object that 
		contains the property to retrieve.
	@param[in] iTag The tag of the property to retrieve.
	@return The value of the property.
	@ingroup jukebox_mom_functions
*/
TJBox_Value JBox_LoadMOMPropertyByTag(TJBox_ObjectRef iObject, TJBox_Tag iTag);

/**
	@brief Update the value of a property.
	@details
		Only properties in the rt_owner scope can be updated.
		The updated value can be read back directly (using, e.g.,
		JBox_LoadMOMProperty), but it will not be propagated
		to the Motherboard until JBox_Export_RenderRealtime 
		returns.
		Updating the value of an invalid property will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iObject The Motherboard property set object that contains
		the property to updated.
	@param[in] iTag The tag of the property to update.
	@param[in] iValue The new value.
	@ingroup jukebox_mom_functions
*/
void JBox_StoreMOMPropertyByTag(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value iValue);


/* TJBox_Value getters and setters. */

/**
	@brief Returns the native type of a Jukebox dynamic value.
	@param[in] iValue The value to examine.
*/
TJBox_ValueType JBox_GetType(TJBox_Value iValue);


/**
	@brief Returns the specified dynamic value as a
		TJBox_Float64. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
*/
TJBox_Float64 JBox_GetNumber(TJBox_Value iValue);

/**
	@brief Returns the length of the specified dynamic value,
		assuming it represents a string. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
*/
TJBox_UInt32 JBox_GetStringLength(TJBox_Value iValue);

/**
	@brief Return a substring from the specified dynamic value,
		assuming it represents a string.
	@details
		Copies the eight-bit UTF-8 characters in the range iStart (inclusive)
		to iEnd (non-inclusive) to the buffer specified by oString. The 
		resulting string is zero-terminated automatically.
		Calling with an empty range is allowed, and returns a single
		terminating zero.
		The oString buffer is assumed to have enough space to store the
		substring (plus a terminating zero character).
		If the value is not of the correct type, the host will terminate
		the 45.
	@param[in] iValue The value to copy from.
	@param[in] iStart First index (inclusive) to copy from.
	@param[in] iEnd Last index (non-inclusive) to copy from.
	@param[out] oString Array in which to store the substring.
*/
void JBox_GetSubstring(
	TJBox_Value iValue, 
	TJBox_SizeT iStart, 
	TJBox_SizeT iEnd,
	char oString[]);

/**
	@brief Returns the specified dynamic value as a
		TJBox_Bool. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
*/
TJBox_Bool JBox_GetBoolean(TJBox_Value iValue);

/**
	@brief Obtain a read-only pointer to a native object.
	@details
		Native objects are created by calls to JBox_Export_CreateNativeObject.
		All native objects are owned by the host. The pointer returned by
		this function can not be stored; it is only valid during the execution
		of the current calling context.
		This function is only valid for read-only native objects that can be
		shared between 45 instances.
	@param[in] iValue The dynamic value that represents the native object.
	@return Pointer to the native object.
	@see JBox_Export_CreateNativeObject()
*/
const void* JBox_GetNativeObjectRO(TJBox_Value iValue);

/**
	@brief Obtain a read/write pointer to a native object.
	@details
		Native objects are created by calls to JBox_Export_CreateNativeObject.
		All native objects are owned by the host. The pointer returned by
		this function can not be stored; it is only valid during the execution
		of the current calling context.
		This function is only valid for read/write native objects. The
		objects cannot be shared between 45 instances: each instance
		must have its own local copy.
	@param[in] iValue The dynamic value that represents the native object.
	@return Pointer to the native object.
*/
void* JBox_GetNativeObjectRW(TJBox_Value iValue);


/**
	@brief Returns the sample-related information for a
		dynamic value, assuming it is a sample. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
	@see JBox_GetSampleData()
*/
TJBox_SampleInfo JBox_GetSampleInfo(TJBox_Value iValue);

/**
	@brief Returns the sample-related meta data for a
		dynamic value, assuming it is a sample. 
		The difference compared to JBox_GetSampleInfo is that sample
		parameters and sample state are also included.
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
	@see JBox_GetSampleData()
*/
TJBox_SampleMetaData JBox_GetSampleMetaData(TJBox_Value iValue);
/**
	@brief Return a range of samples from the specified dynamic value.
	@details
		Copies the sample values in the range iStartFrame (inclusive)
		to iEndFrame (non-inclusive) to the buffer specified by oAudio. 
		Each frame contains one TJBox_AudioSample per channel. Channel
		data is interleaved in increasing order. A stereo frame is two
		TJBox_AudioSample:s long. Retrieve the TJBox_SampleInfo (if the
		sample is a custom_property) or TJBox_SampleMetaData (if it is a
		user_sample item) for the sample to obtain information about the
		number of channels.
		Calling with an empty range has no effect.
		The oAudio buffer is assumed to have enough space to store the
		samples: (iEndFrame-iStartFrame)*channelCount.
		If the value is not of the correct type, the host will terminate
		the 45.
		
		The sample data is assumed to be resident in the range given by iStartFrame
		and iEndFrame. The resident frame count should be checked by calling
		JBox_GetSampleInfo prior to calling JBox_GetSampleData.
	@param[in] iValue The value to copy from.
	@param[in] iStartFrame First index (inclusive) to copy from.
	@param[in] iEndFrame Last index (non-inclusive) to copy from.
	@param[out] oAudio Array in which to store the samples.
	@see JBox_GetSampleInfo()
*/
void JBox_GetSampleData(
	TJBox_Value iValue,
	TJBox_AudioFramePos iStartFrame,
	TJBox_AudioFramePos iEndFrame,
	TJBox_AudioSample oAudio[]);

/**
	@brief Returns the BLOB-related information for a
		dynamic value, assuming it is a BLOB. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
	@see JBox_GetBLOBData()
*/
TJBox_BLOBInfo JBox_GetBLOBInfo(TJBox_Value iValue);

/**
	@brief Return a range of bytes from the specified dynamic value.
	@details
		Copies the bytes in the range iStartFrame (inclusive)
		to iEndFrame (non-inclusive) to the buffer specified by oData. 
		Calling with an empty range has no effect.
		The oData buffer is assumed to have enough space to store the
		bytes: (iEnd - iStart).
		If the value is not of the correct type, the host will terminate
		the 45.
	@param[in] iValue The value to copy from.
	@param[in] iStart First index (inclusive) to copy from.
	@param[in] iEnd Last index (non-inclusive) to copy from.
	@param[out] oData Array in which to store the bytes.
	@see JBox_GetBLOBInfo()
*/
void JBox_GetBLOBData(
	TJBox_Value iValue,
	TJBox_SizeT iStart,
	TJBox_SizeT iEnd,
	TJBox_UInt8 oData[]);


/**
	@brief Returns the DSPBuffer-related information for a
		dynamic value, assuming it is a DSP buffer. 
	@details If the value is not of the correct type, the
		host will terminate the 45.
	@param[in] iValue The value to examine.
	@see JBox_GetDSPBufferData()
*/
TJBox_DSPBufferInfo JBox_GetDSPBufferInfo(TJBox_Value iValue);

/**
	@brief Return a range of samples from the specified dynamic value.
	@details
		Copies the sample values in the range iStartFrame (inclusive)
		to iEndFrame (non-inclusive) to the buffer specified by oAudio. 
		Each DSP buffer frame always contains one TJBox_AudioSample.
		Calling with an empty range has no effect.
		The oAudio buffer is assumed to have enough space to store the
		samples: (iEndFrame-iStartFrame).
		If the value is not of the correct type, the host will terminate
		the 45.
	@param[in] iValue The value to copy from.
	@param[in] iStartFrame First index (inclusive) to copy from.
	@param[in] iEndFrame Last index (non-inclusive) to copy from.
	@param[out] oAudio Array in which to store the samples.
	@see JBox_GetDSPBufferInfo()
*/
void JBox_GetDSPBufferData(
	TJBox_Value iValue,
	TJBox_AudioFramePos iStartFrame,
	TJBox_AudioFramePos iEndFrame,
	TJBox_AudioSample oAudio[]);

/**
	@brief Writes a range of samples from a local buffer to a dynamic
	value, assuming it is a DSP buffer.
	@details
		Copies the sample values from the iAudio buffer to the range 
		iStartFrame (inclusive) to iEndFrame (non-inclusive) in the
		DSP buffer.
		Each DSP buffer frame always contains one TJBox_AudioSample.
		The specified range must fit in the DSPBuffer; the DSPBuffer will
		not be resized. Retrieve the TJBox_DSPBufferInfo for the
		DSP buffer to obtain information about its size.
		Calling with an empty range has no effect.
		If the value is not of the correct type, the host will terminate
		the 45.
		Jukebox allows DSP buffers to be modified by the real-time code; 
		no other dynamic value-types may be modified (only replaced).
	@param[in] iValue The value to copy to.
	@param[in] iStartFrame First index (inclusive) to copy to.
	@param[in] iEndFrame Last index (non-inclusive) to copy to.
	@param[in] iAudio Array from which to copy the samples.
	@see JBox_GetDSPBufferInfo()
*/
void JBox_SetDSPBufferData(
	TJBox_Value iValue,
	TJBox_AudioFramePos iStartFrame,
	TJBox_AudioFramePos iEndFrame,
	const TJBox_AudioSample iAudio[]);

/**
	@brief Sets the contents of an RT owned string.
	@details
		Copies iSize bytes from iData to the RT string.
		iSize cannot be larger than the max_size defined in the motherboard
		If the value is not of the correct type, the host will terminate the 45.
		Jukebox allows RT owned strings to be modified by the real-time code;
		no other dynamic value-type except DSP Buffers may be modified (only replaced).
		The maximum supported rate of updates to a single value is once every
		25 milliseconds. If this rate is exceeded, the host will drop some updates
		and warn in the log (and blow fuse if blow fuse on warnings is enabled)
	@param[in] iProperty The RT string property to update
	@param[in] iSize Number of bytes to copy
	@param[in] iData Array from which to copy the bytes.
*/
void JBox_SetRTStringData(TJBox_PropertyRef iProperty, TJBox_SizeT iSize, const TJBox_UInt8 iData[]);

/**
	@brief Sends a note event to the host.
	@details
		The specified note event will be sent to the host.
		An event with zero velocity turns off a note.
		Events with non-zero velocity turns on a note.
		This function can only be used by note player devices.

	@param[in] iNoteEvent The note event to be outputted.
*/
void JBox_OutputNoteEvent(TJBox_NoteEvent iNoteEvent);

/**
	@brief Utility function to convert a note state property diff to a note event.
	@details
		This function assumes that the property diff is a note statet property diff
		with a tag that represents the note number (0-127) and a current value that
		represents the velocity (0-127). The frame index of the property diff must be
		a value between 0-63. Passing a property diff with other values, will be
		treated as a programming error.
		This function can be used by device of all types.
	@param[in] iPropertyDiff The property diff of a note state.
	@return The note event based on the values from the property diff.
*/
TJBox_NoteEvent JBox_AsNoteEvent(const TJBox_PropertyDiff& iPropertyDiff);

/*
	NOTE: There is no way to make a new string, sample, BLOB or DSP-buffer
	from real-time code, you do that in your data model script.
	You can copy TJBox_Value:s, but you cannot keep values that refer to
	TJBox_Value instances between invocations to JBox_Export_RenderRealtime
	or JBox_Export_CreateNativeObject.
*/

/**
	@brief Return an empty dynamic value.
*/
TJBox_Value JBox_MakeNil();

/**
	@brief Return a dynamic value that represents a number.
	@param[in] iNumber The number to store in the dynamic value.
*/
TJBox_Value JBox_MakeNumber(TJBox_Float64 iNumber);

/**
	@brief Return a dynamic value that represents a boolean.
	@param[in] iBoolean The bool to store in the dynamic value.
*/
TJBox_Value JBox_MakeBoolean(TJBox_Bool iBoolean);

/* Utilities. */

/**
	@brief Retrieve a number from a specified property.
	@details
		Requesting the value of an invalid property, or if the
		property is of the incorrect type, will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iObject The Motherboard property set object that 
		contains the property to retrieve.
	@param[in] iTag The tag of the property to retrieve.
	@return The value of the property.
	@ingroup jukebox_mom_functions
*/
TJBox_Float64 JBox_LoadMOMPropertyAsNumber(
	TJBox_ObjectRef iObject, 
	TJBox_Tag iTag);

/**
	@brief Update the value of a property, assuming it is a 
		number.
	@details
		Only properties in the rt_owner scope can be updated.
		The updated value can be read back directly (using, e.g.,
		JBox_LoadMOMProperty), but it will not be propagated
		to the Motherboard until JBox_Export_RenderRealtime 
		returns.
		Updating the value of an invalid property, or if the
		property is of the incorrect type, will terminate 
		the 45 instance.
		Cannot be called from JBox_Export_CreateNativeObject.
	@param[in] iObject The Motherboard property set object that 
		contains the property to updated.
	@param[in] iTag The tag of the property to update.
	@param[in] iValue The new value.
	@ingroup jukebox_mom_functions
*/
void JBox_StoreMOMPropertyAsNumber(
	TJBox_ObjectRef iObject, 
	TJBox_Tag iTag, 
	TJBox_Float64 iValue);


/* Memory alignment functions */

/**
	@brief Tells the optimal alignment for JBox_FFTRealForward and JBox_FFTRealInverse.
	@details
		Returns a power-of-two number >= 16 (16 is the base alignment of Jukebox).
		
		Function prototype: 

		TJBox_Int32 JBox_GetOptimalFFTAlignment();

	@see JBox_FFTRealForward(), JBox_FFTRealInverse()
*/
TJBox_Int32 JBox_GetOptimalFFTAlignment();


/* Accelerated DSP functions. */

/**
	@brief Implements the real Fast Fourier Transform (time domain to
		frequency domain for real-valued signals).
	@details
		Performs in-place forward FFT of real valued single precision float data.

		The transform is calculated in-place by reading N consecutive real values and
		writing N/2 interlaced (re, im) complex pairs. All scaling is handled internally,
		there is no windowing applied.

		Input values re0, re1, re2, re3... produces output values re0, im0, re1, im1...

		Works on power-of-two-sized buffers.
	@param[in] iFFTSize
		Size of the FFT, expressed as the log2 of the input data array 
		size. For example, iFFTSize = 6 corresponds to an input data 
		array size of 64. iFFTSize must be larger than or equal to 6 
		and must be smaller than or equal to 15.
	@param[inout] ioData
		Array that contains the input signal to transform. Will contain
		the resulting transformed frequency-domain data when this
		function returns. The output values will be divided by the FFT
		length. The address of ioData must be aligned to 16-bytes.
		For optimal performance, align ioData according to JBox_GetOptimalFFTAlignment().
	@see JBox_GetOptimalFFTAlignment()
*/
void JBox_FFTRealForward(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[]);

/**
	@brief Implements the real Inverse Fast Fourier Transform 
		(frequency domain to time domain for real-valued signals).
	@details
		Performs in-place inverse FFT of single precision float data.

		The transform is calculated in-place by reading N/2 interlaced
		complex pairs and writing N consecutive real values. All scaling
		is handled internally, there is no windowing applied.

		Input values re0, im0, re1, im1... produces output values re0, re1, re2, re3...

		Works on power-of-two-sized buffers.
	@param[in] iFFTSize
		Size of the FFT, expressed as the log2 of the input data array 
		size. For example, iFFTSize = 6 corresponds to an input data 
		array size of 64. iFFTSize must be larger than or equal to 6 
		and must be smaller than or equal to 15.
	@param[inout] ioData
		Array that contains the input signal to transform. Will contain
		the resulting transformed time-domain data when this function
		returns. The output values will be divided by the FFT length.
		The address of ioData must be aligned to 16-bytes.
		For optimal performance, align ioData according to JBox_GetOptimalFFTAlignment().
	@see JBox_GetMemoryAligment()
*/
void JBox_FFTRealInverse(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[]);




#ifdef __cplusplus
}
#endif


/**
	@brief Constant that is used to check for silent audio samples.
	@details
		If the following expression is true the sample should be 
		considered silent:
		
		abs(sample) < kJBox_SilentThreshold
*/
const TJBox_Float32 kJBox_SilentThreshold = ((TJBox_Float32)2.0e-8f);


#endif /* PROPELLERHEAD_JUKEBOX_H */
