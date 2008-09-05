/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004-2008 Grame
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
 */

#ifndef __JackDriver__
#define __JackDriver__

#include "types.h"
#include "JackClientInterface.h"
#include "JackConstants.h"
#include "JackPlatformPlug.h"
#include "JackClientControl.h"
#include <list>

namespace Jack
{
    
class JackLockedEngine;
class JackGraphManager;
struct JackEngineControl;
    
/*!
\brief The base interface for drivers.
*/
    
class EXPORT JackDriverInterface
{

    public:
        
        JackDriverInterface()
        {}
        virtual ~JackDriverInterface()
        {}
        
        virtual int Open() = 0;
        virtual int Open(jack_nframes_t buffer_size,
                         jack_nframes_t samplerate,
                         bool capturing,
                         bool playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency) = 0;
        
        virtual int Attach() = 0;
        virtual int Detach() = 0;
        
        virtual int Read() = 0;
        virtual int Write() = 0;
        
        virtual int Start() = 0;
        virtual int Stop() = 0;
        
        virtual bool IsFixedBufferSize() = 0;
        virtual int SetBufferSize(jack_nframes_t buffer_size) = 0;
        virtual int SetSampleRate(jack_nframes_t sample_rate) = 0;
        
        virtual int Process() = 0;
        virtual int ProcessNull() = 0;
        
        virtual void SetMaster(bool onoff) = 0;
        virtual bool GetMaster() = 0;
        virtual void AddSlave(JackDriverInterface* slave) = 0;
        virtual void RemoveSlave(JackDriverInterface* slave) = 0;
        virtual int ProcessSlaves() = 0;
        
        virtual bool IsRealTime() const = 0;
};

/*!
 \brief The base interface for drivers clients.
 */

class EXPORT JackDriverClientInterface : public JackDriverInterface, public JackClientInterface
{};

/*!
 \brief The base class for drivers.
 */

class EXPORT JackDriver : public JackDriverClientInterface
{
    
    protected:
        
        char fCaptureDriverName[JACK_CLIENT_NAME_SIZE + 1];
        char fPlaybackDriverName[JACK_CLIENT_NAME_SIZE + 1];
        char fAliasName[JACK_CLIENT_NAME_SIZE + 1];
        jack_nframes_t fCaptureLatency;
        jack_nframes_t fPlaybackLatency;
        jack_time_t fBeginDateUst;
        jack_time_t fEndDateUst;
        float fDelayedUsecs;
        JackLockedEngine* fEngine;
        JackGraphManager* fGraphManager;
        JackSynchro* fSynchroTable;
        JackEngineControl* fEngineControl;
        JackClientControl fClientControl;
        std::list<JackDriverInterface*> fSlaveList;
        bool fIsMaster;
           
        void CycleIncTime();
        void CycleTakeBeginTime();
        void CycleTakeEndTime();
        
        void SetupDriverSync(int ref, bool freewheel);
        
        void NotifyXRun(jack_time_t callback_usecs, float delayed_usecs);   // XRun notification sent by the driver
        void NotifyBufferSize(jack_nframes_t buffer_size);                  // BufferSize notification sent by the driver
        void NotifySampleRate(jack_nframes_t sample_rate);                  // SampleRate notification sent by the driver
 
    public:
        
        JackDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        JackDriver();
        virtual ~JackDriver();
        
        void SetMaster(bool onoff);
        bool GetMaster();
        void AddSlave(JackDriverInterface* slave);
        void RemoveSlave(JackDriverInterface* slave);
        int ProcessSlaves();
      
        virtual int Open();
        virtual int Open(jack_nframes_t buffer_size,
                         jack_nframes_t samplerate,
                         bool capturing,
                         bool playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency);
        virtual int Close();
        
        virtual int Process();
        virtual int ProcessNull();
        
        virtual int Attach();
        virtual int Detach();
         
        virtual int Read();
        virtual int Write();
          
        virtual int Start();
        virtual int Stop();
        
        virtual bool IsFixedBufferSize();
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetSampleRate(jack_nframes_t sample_rate);
        
        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, int value1, int value2);
        virtual JackClientControl* GetClientControl() const;
        
        virtual bool IsRealTime() const;
        virtual bool Init();  // To be called by the wrapping thread Init method when the driver is a "blocking" one 
            
};
    
} // end of namespace

#endif
