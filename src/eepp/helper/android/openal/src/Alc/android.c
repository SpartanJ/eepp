/**
 * OpenAL cross platform audio library
 * Copyright (C) 2010 by Chris Robinson
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <stdlib.h>
#include <jni.h>
#include <pthread.h>
#include "alMain.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/android.h"

static int doPause=0;
int resumeHandled;
int pauseHandled;

static const ALCchar android_device[] = "Android Default";

static JavaVM* javaVM = NULL;

static jclass cAudioTrack = NULL;

static jmethodID mAudioTrack;
static jmethodID mGetMinBufferSize;
static jmethodID mPlay;
static jmethodID mStop;
static jmethodID mRelease;
static jmethodID mWrite;
static jmethodID mPause;

void AL_SetJavaVM( void * vm )
{
	javaVM = (JavaVM*)vm;
}

/*
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    javaVM = vm;
    return JNI_VERSION_1_2;
}
*/

static JNIEnv* GetEnv()
{
    JNIEnv* env = NULL;
    if (javaVM) (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_2);
    return env;
}

typedef struct
{
    pthread_t thread;
    volatile int running;
} AndroidData;

#define STREAM_MUSIC 3
#define CHANNEL_CONFIGURATION_MONO 2
#define CHANNEL_CONFIGURATION_STEREO 3
#define ENCODING_PCM_8BIT 3
#define ENCODING_PCM_16BIT 2
#define MODE_STREAM 1

static void* thread_function(void* arg)
{
    ALCdevice* device = (ALCdevice*)arg;
    AndroidData* data = (AndroidData*)device->ExtraData;

    JNIEnv* env;
    (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);

    (*env)->PushLocalFrame(env, 2);

    int sampleRateInHz = device->Frequency;
    int channelConfig = aluChannelsFromFormat(device->Format) == 1 ? CHANNEL_CONFIGURATION_MONO : CHANNEL_CONFIGURATION_STEREO;
    int audioFormat = aluBytesFromFormat(device->Format) == 1 ? ENCODING_PCM_8BIT : ENCODING_PCM_16BIT;

    int bufferSizeInBytes = (*env)->CallStaticIntMethod(env, cAudioTrack, 
        mGetMinBufferSize, sampleRateInHz, channelConfig, audioFormat);

    int bufferSizeInSamples = bufferSizeInBytes / aluFrameSizeFromFormat(device->Format);

    jobject track = (*env)->NewObject(env, cAudioTrack, mAudioTrack,
        STREAM_MUSIC, sampleRateInHz, channelConfig, audioFormat, device->NumUpdates * bufferSizeInBytes, MODE_STREAM);

	//(*env)->CallNonvirtualVoidMethod(env, track, cAudioTrack, mPlay);

	jarray buffer = (*env)->NewByteArray(env, bufferSizeInBytes);

	pauseHandled=0;
	resumeHandled=0;
	while (data->running)
	{
		if(doPause && !pauseHandled)
		{
			(*env)->CallNonvirtualVoidMethod(env, track, cAudioTrack, mPause);
			pauseHandled=1;
		}
		if(!doPause && !resumeHandled)
		{
			(*env)->CallNonvirtualVoidMethod(env, track, cAudioTrack, mPlay);
			resumeHandled=1;
		}
		
		void* pBuffer = (*env)->GetPrimitiveArrayCritical(env, buffer, NULL);

		if (pBuffer)
		{
			aluMixData(device, pBuffer, bufferSizeInSamples);
			(*env)->ReleasePrimitiveArrayCritical(env, buffer, pBuffer, 0);

			(*env)->CallNonvirtualIntMethod(env, track, cAudioTrack, mWrite, buffer, 0, bufferSizeInBytes);
		}
		else
		{
			AL_PRINT("Failed to get pointer to array bytes");
		}
	}
	
	(*env)->CallNonvirtualVoidMethod(env, track, cAudioTrack, mStop);
	(*env)->CallNonvirtualVoidMethod(env, track, cAudioTrack, mRelease);

    (*env)->PopLocalFrame(env, NULL);

    (*javaVM)->DetachCurrentThread(javaVM);
    return NULL;
}

static ALCboolean android_open_playback(ALCdevice *device, const ALCchar *deviceName)
{
    JNIEnv* env = GetEnv();
    AndroidData* data;
    int channels;
    int bytes;

    if (!cAudioTrack)
    {
        /* Cache AudioTrack class and it's method id's
         * And do this only once!
         */
        cAudioTrack = (*env)->FindClass(env, "android/media/AudioTrack");
        if (!cAudioTrack)
        {
            AL_PRINT("android.media.AudioTrack class is not found. Are you running at least 1.5 version?");
            return ALC_FALSE;
        }

        cAudioTrack = (*env)->NewGlobalRef(env, cAudioTrack);

        mAudioTrack = (*env)->GetMethodID(env, cAudioTrack, "<init>", "(IIIIII)V");
        mGetMinBufferSize = (*env)->GetStaticMethodID(env, cAudioTrack, "getMinBufferSize", "(III)I");
        mPlay = (*env)->GetMethodID(env, cAudioTrack, "play", "()V");
		mPause = (*env)->GetMethodID(env, cAudioTrack, "pause", "()V");
        mStop = (*env)->GetMethodID(env, cAudioTrack, "stop", "()V");
        mRelease = (*env)->GetMethodID(env, cAudioTrack, "release", "()V");
        mWrite = (*env)->GetMethodID(env, cAudioTrack, "write", "([BII)I");
    }

    if (!deviceName)
    {
        deviceName = android_device;
    }
    else if (strcmp(deviceName, android_device) != 0)
    {
        return ALC_FALSE;
    }

    data = (AndroidData*)calloc(1, sizeof(*data));
    device->szDeviceName = strdup(deviceName);
    device->ExtraData = data;
    return ALC_TRUE;
}

static void android_close_playback(ALCdevice *device)
{
    AndroidData* data = (AndroidData*)device->ExtraData;
    if (data != NULL)
    {
        free(data);
        device->ExtraData = NULL;
    }
}

static ALCboolean android_reset_playback(ALCdevice *device)
{
    AndroidData* data = (AndroidData*)device->ExtraData;

    if (aluChannelsFromFormat(device->Format) >= 2)
    {
        device->Format = aluBytesFromFormat(device->Format) >= 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
    }
    else
    {
        device->Format = aluBytesFromFormat(device->Format) >= 2 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
    }

    SetDefaultChannelOrder(device);

    data->running = 1;
    pthread_create(&data->thread, NULL, thread_function, device);

    return ALC_TRUE;
}

static void android_stop_playback(ALCdevice *device)
{
    AndroidData* data = (AndroidData*)device->ExtraData;

    if (data->running)
    {
        data->running = 0;
        pthread_join(data->thread, NULL);
    }
}

static ALCboolean android_open_capture(ALCdevice *pDevice, const ALCchar *deviceName)
{
    (void)pDevice;
    (void)deviceName;
    return ALC_FALSE;
}

static void android_close_capture(ALCdevice *pDevice)
{
    (void)pDevice;
}

static void android_start_capture(ALCdevice *pDevice)
{
    (void)pDevice;
}

static void android_stop_capture(ALCdevice *pDevice)
{
    (void)pDevice;
}

static void android_capture_samples(ALCdevice *pDevice, ALCvoid *pBuffer, ALCuint lSamples)
{
    (void)pDevice;
    (void)pBuffer;
    (void)lSamples;
}

static ALCuint android_available_samples(ALCdevice *pDevice)
{
    (void)pDevice;
    return 0;
}


static const BackendFuncs android_funcs = {
    android_open_playback,
    android_close_playback,
    android_reset_playback,
    android_stop_playback,
    android_open_capture,
    android_close_capture,
    android_start_capture,
    android_stop_capture,
    android_capture_samples,
    android_available_samples
};

void alc_android_init(BackendFuncs *func_list)
{
    *func_list = android_funcs;
}

void alc_android_deinit(void)
{
    JNIEnv* env = GetEnv();

    /* release cached AudioTrack class */
    (*env)->DeleteGlobalRef(env, cAudioTrack);
}

void alc_android_probe(int type)
{
    if (type == DEVICE_PROBE)
    {
        AppendDeviceList(android_device);
    }
    else if (type == ALL_DEVICE_PROBE)
    {
        AppendAllDeviceList(android_device);
    }
}
AL_API void AL_APIENTRY al_android_pause_playback()
{
	doPause=1;
	pauseHandled=0;
	AL_PRINT("Audio paused.");
}
AL_API void AL_APIENTRY al_android_resume_playback()
{
	doPause=0;
	resumeHandled=0;
	AL_PRINT("Audio resumed.");
}