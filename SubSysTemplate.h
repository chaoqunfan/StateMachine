/**@file 
 * Copyright (C) 2013, Sinovision Tech Ltd.
 * All rights reserved.
 *
 * @brief   This file defines the top level state machine for each subsytem manager
 *
 * @author  
 * @date 2013-4-19
 *
 */
 
#ifndef _SUBSYS_TEMPLATE_H_
#define _SUBSYS_TEMPLATE_H_

#include "ecl_events.h"
#include "ECService.h"
#include "Macho.h"

using namespace Macho;

/* the template class defines the event Interface for each subsystem
*/
template <class I>
class SubSysTemplate : public ECService
{
public:
	explicit SubSysTemplate(SVCID id, int priority, \
        bool detach, size_t stack, os_thread_data_t arg = NULL)
        :ECService(id, priority, detach, stack, arg){};
	~SubSysTemplate(){};



	virtual int selfTest()
	{
		// when the connection state management did not finished, this event is to move the sm to connected state
		//printf("selfTest\n");
		//sendEvent(getID(),EV_ECL_CONNECTED);
		return 0;
	}

	virtual int initSubSys()
	{
		//sendEvent(getID(),EV_SS_INIT_DONE);
		return 0;
	}

protected:

	//those marcos are needed to avoid the compile error and keep the same style interface as the macho defines
	#define STATE_T(S) STATE_TEMPLATE(S, Top) 
	#define STATE_C(S) STATE_TEMPLATE(S, Connected) 
	#define TOP TopBase< Top >::TOP 
	#define setState _StateSpecification::setState
	#define setStateHistory _StateSpecification::setStateHistory
	
	TOPSTATE(Top),public I {
		// Top state variables (visible to all substates)
		struct Box {
			Box() :debug(true){}
			SVCID sid; //service id
			SVCID sc; //subsytem control service id in scan control		
			SubSysTemplate * ssp; //sub system referencce
			bool debug;
		};

		STATE_TEMPLATE(Top, TopBase< Top >)

		// Machine's event protocol
		virtual void evSubSysStart() {}
		virtual void evSubSysInitDone() {}			
		virtual void evSubSysShutdown() {}
		virtual void evSubSysFault() {}
		
		virtual void evSubSysInitFailed() {}			
		virtual void evSubSysFaultRecovered() {}
		virtual void evSubSysConnecteded(){}
		virtual void evSubSysDisConnecteded(){}
		virtual void evSubSysSelfTestFailed(){}

		virtual void evEclConnecteded(EV_ECL_CONNECTED_TYPE *event){}
		virtual void evEclDisconnected(EV_ECL_DISCONNECTED_TYPE *event){}
		
		void start(){setState<PowerUp>();};

	private:
		// special actions
		void entry(){}
		void exit(){}
	};

	SUBSTATE(PowerUp, Top) {
		struct Box {
		};
		STATE_T(PowerUp)

		void evSubSysSelfTestFailed(){setState<Shutdown>();};
		void evSubSysConnecteded(){setState<WaitingForStart>();}

	private:
		// Entry and exit actions of state
		void entry()
		{	
			TOP::box().ssp->selfTest();
		}
		void exit(){};

		
	};

	SUBSTATE(Shutdown, Top) {
		struct Box {
		};
		STATE_T(Shutdown)
			
		void evSubSysStart() {
			setState<Initializing>();
		}
	private:
		void entry() {}
		void init();

	};

	SUBSTATE(Disconnected, Top) {
		struct Box {
		};
		STATE_T(Disconnected)

		void evSubSysConnecteded()
		{
			setState<Connected>();//modify by wjw setStateHistory<Connected>();
		}

	private:
		void init();
		// Entry and exit actions of state
		void entry(){}
		void exit(){}
		
	};


	SUBSTATE(Connected, Top) {
		struct Box {
		};
		STATE_T(Connected)
		HISTORY()

		void evSubSysDisConnecteded()
		{
			setState<Disconnected>();
		}
		
		void evSubSysShutdown()
		{
			setState<Shutdown>();
		}

	private:
		// Entry and exit actions of state
		void entry(){}
		void exit(){}
		
		void init() {
			setState<WaitingForStart>();
		}
	};

	SUBSTATE(WaitingForStart, Connected) {
		struct Box {
		};
		STATE_C(WaitingForStart)

		void evSubSysStart(){setState<Initializing>();}

		void evSubSysFault(){setState<Fault>();}
		void evSubSysShutdown()
		{
			setState<Shutdown>();
		}
	private:
		// Entry and exit actions of state
		void entry(){ 
			// notify the scann control subsystem is powered up
			ECService::sendEvent(TOP::box().sc,EV_SS_POWERED);
		}
		void exit(){}
	};

	SUBSTATE(Initializing, Connected) {
		struct Box {
		};

		STATE_C(Initializing)

		void evSubSysInitDone(){setState<Operation>();}		
		void evSubSysInitFailed(){setState<Shutdown>();}

	private:
		// Entry and exit actions of state
		void entry()
		{
			TOP::box().ssp->initSubSys();
		}
		void exit(){}
		void init();
	};

	SUBSTATE(Operation, Connected) {
		struct Box {
		};
		STATE_C(Operation)

		void evSubSysFault(){setState<Fault>();}
	private:
		// Entry and exit actions of state
		void entry(){}
		void exit(){}
		void init();
	};

	SUBSTATE(Fault, Connected) {
		struct Box {
		};
		STATE_C(Fault)

		void evSubSysFaultRecovered()
		{
			setState<Operation>();
		}

		void evSubSysStart() { 
			setState<Initializing>();
		}

	private:
		// Entry and exit actions of state
		void entry(){ 
			// notify the scann control subsystem is in falult state
			ECService::sendEvent(TOP::box().sc,EV_SS_ENTER_FAULT);
		}
		void exit(){ 
			// notify the scann control subsystem is in falult state
			ECService::sendEvent(TOP::box().sc,EV_SS_LEAVE_FAULT);
		}
		void init();
	};

	 Macho::Machine<Top> *m;
	SVCID sc;// subsystem control service id

	/**Operations needed for subsys term manager service thread init
	 */
	virtual int ssThreadInit(){return 0;};

	void cbEvSubSysStart(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysStart();
	}
	
	void cbEvSubSysInitDone(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysInitDone();
		ECService::sendEvent(sc,EV_SS_READY);
	}
	
	void cbEvSubSysShutdown(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysShutdown();
	}
	
	void cbEvSubSysFault(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysFault();
	}

	void cbEvSubSysInitFailed()			
	{
		(*m)->evSubSysInitFailed();
	}
	void cbEvSubSysFaultRecovered()
	{
		(*m)->evSubSysFaultRecovered();
	}
	void cbEvSubSysConnecteded(EV_ECL_CONNECTED_TYPE *event)
	{
	    if((((*m)->box().sc == event->from) && ((*m)->box().sid == event->to)) || \
	    		(((*m)->box().sc == event->to) && ((*m)->box().sid == event->from))){
            	(*m)->evSubSysConnecteded();
		} else { 
			(*m)->evEclConnecteded(event);			
		}
	}

	void cbEvSubSysDisConnecteded(EV_ECL_DISCONNECTED_TYPE *event)
	{
		if((((*m)->box().sc == event->from) && ((*m)->box().sid == event->to)) || \
			    (((*m)->box().sc == event->to) && ((*m)->box().sid == event->from))){
            	(*m)->evSubSysDisConnecteded();
		} else { 
			(*m)->evEclDisconnected(event);
		}
	}

	void cbEvSubSysSelfTestFailed()
	{
		(*m)->evSubSysSelfTestFailed();
	}

	void cbEvQueryVer(EV_SS_QUERY_VER_TYPE *event)
	{
		sendVerToCtl(sc);
	}

	void cbEvFpgaReportVer(EV_SS_FPGA_REPORT_VER_TYPE *ev)
	{
		setFpgaVer(ev);
	}
	//void cbSvRequestStatus(EVENT_HEADER_TYPE *event)
	//{
	//	printf("event->sid:%d\n",event->sid);
	//	ECService::sendEvent(event->sid, EV_SV_RESPONSE_STATUS);
	//}
	
	int initialize(){return 0;}

public:
    const char* currentState()
    {
        Alias state = (*m).currentState();
        return state.name();
    }

    

private:	
	int threadInitialize()
	{
		registerEvent(EV_SS_START,(EventHandler)&SubSysTemplate::cbEvSubSysStart);
		registerEvent(EV_SS_INIT_DONE,(EventHandler)&SubSysTemplate::cbEvSubSysInitDone);
		registerEvent(EV_SS_SHUTDOWN,(EventHandler)&SubSysTemplate::cbEvSubSysShutdown);
		registerEvent(EV_SS_FAULT,(EventHandler)&SubSysTemplate::cbEvSubSysFault);
		registerEvent(EV_SS_INIT_FAIL,(EventHandler)&SubSysTemplate::cbEvSubSysInitFailed);
		registerEvent(EV_SS_FAULT_RECOVERED,(EventHandler)&SubSysTemplate::cbEvSubSysFaultRecovered);
		registerEvent(EV_ECL_CONNECTED,(EventHandler)&SubSysTemplate::cbEvSubSysConnecteded);
		registerEvent(EV_ECL_DISCONNECTED,(EventHandler)&SubSysTemplate::cbEvSubSysDisConnecteded);
		registerEvent(EV_SS_SELF_TEST_FAIL,(EventHandler)&SubSysTemplate::cbEvSubSysSelfTestFailed);
		registerEvent(EV_SS_QUERY_VER,(EventHandler)&SubSysTemplate::cbEvQueryVer);
		registerEvent(EV_SS_FPGA_REPORT_VER,(EventHandler)&SubSysTemplate::cbEvFpgaReportVer);
		//registerEvent(EV_SV_REQUEST_STATUS,(EventHandler)&SubSysTemplate::cbSvRequestStatus);
		ssThreadInit();
		(*m)->start();        
		return 0;
	}
};


#endif


















