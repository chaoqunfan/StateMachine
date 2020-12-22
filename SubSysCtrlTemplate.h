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
 

#ifndef _SUBSYS_CTRL_TEMPLATE_H_
#define _SUBSYS_CTRL_TEMPLATE_H_

#include "ecl_events.h"
#include "ECService.h"
#include "CmErrorStruct.h"
#include "Macho.h"
#ifdef UNIT_TEST
#include "../Appl/ScGlobal.h"
#include "../Appl/ScDataBase.h"
#else
#include "ScGlobal.h"
#include "ScDataBase.h"
#endif
using namespace Macho;



/* the template class defines the event Interface for each subsystem
*/
template <class I>
class SubSysCtrlTemplate : public ECService
{


public:
	explicit SubSysCtrlTemplate(SVCID id, int priority,
        bool detach, size_t stack,os_thread_data_t arg = NULL)
        :ECService(id, priority, detach, stack, arg)
        {

            eh = SVC_ID_SC_ERR_SVC;

        }

	~SubSysCtrlTemplate(){}



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
			Box(){
                panel = SVC_ID_SC_PANEL_CTRL;
                db = SVC_ID_SC_DISP_CTL;
                eh = SVC_ID_SC_ERR_SVC;
                pet =SVC_ID_PET_CTRL;
                }
			SVCID sid; //this service id
			SubSysCtrlTemplate * ssc; //sub system controller reference
			SVCID ss; // subsystem service id
			SVCID seq;// scan sequence control service id
			SVCID panel;// gantry panel/ctbox control service id
			SVCID db;// database service id
   			SVCID eh;// error handling service id
   			SVCID pet;

		};

		STATE_TEMPLATE(Top, TopBase< Top >)

		// Machine's event protocol
		virtual void evSubSysPowered(){}		
		virtual void evSubSysConnecteded(){}
		virtual void evSubSysDisConnecteded(){}
		virtual void evSubSysReady() {}
		virtual void evSubSysEnterFault() {}
		virtual void evSubSysLeaveFault() {}
		virtual void evSubSysEnterShutdown() {}
		virtual void evErrorReporting(EV_ERROR_REPORTING_TYPE *event){}
		virtual void evSVCFaultRecover(){}
		virtual	void evSubSysVersionResp(EV_XML_VERSION_TYPE *event){}
		
		void start()
		{
			setState<PowerUp>();
		}

	private:
		// special actions
		void entry(){}
		void exit(){}
	};

	SUBSTATE(PowerUp, Top) {
		struct Box {
		};
		STATE_T(PowerUp)

		void evSubSysConnecteded(){setState<Connected>();}

	private:
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
		void evSubSysPowered()
		{
			setState<Initializing>();
		}
		void evSubSysEnterShutdown()
		{
			setState<Shutdown>();
		}

	private:
		// Entry and exit actions of state
		void init()
		{
		    setState<Initializing>();
		}
		void entry(){}
		void exit(){}
	};

	SUBSTATE(Disconnected, Top) {
		struct Box {
		};
		STATE_T(Disconnected)

		void evSubSysConnecteded()
		{
			//setStateHistory<Connected>();
			setState<Connected>();
		}

	private:
		// Entry and exit actions of state
		void entry(){}
		void exit(){}
	};


	SUBSTATE(Initializing, Connected) {
		struct Box {
		};
		STATE_C(Initializing)

		void evSubSysReady(){
			ECService::sendEvent(TOP::box().ss, EV_SS_QUERY_VER);
		}

		void evSubSysVersionResp(EV_XML_VERSION_TYPE *event){setState<Operation>();}
		void evSubSysPowered()
		{
			// empty method here to avoid extra setstate Initializing
		}
	private:
		// Entry and exit actions of state
		void entry()
		{
			ECService::sendEvent(TOP::box().ss,EV_SS_START);
		}
		void exit(){}
    void init();
	};

	SUBSTATE(Operation, Connected) {
		struct Box {
		};
		STATE_C(Operation)

		void evSubSysEnterFault(){setState<Fault>();}
		void evErrorReporting(EV_ERROR_REPORTING_TYPE *event){}
	private:
		// Entry and exit actions of state
		void entry(){
			ECService::logMsg(LOG_LEVEL_INFO,"subsystem %d is ready. \n",TOP::box().ss);
		}
		void exit();
		void init();
	};

	SUBSTATE(Fault, Connected) {
		struct Box {
		};
		STATE_C(Fault)

		void evSubSysLeaveFault()
		{
			setState<Operation>();
		}

		void evSVCFaultRecover()
		{
			setState<Initializing>();
		}


	private:
		// Entry and exit actions of state
		void init();
		void entry(){}
		void exit(){}
	};

	SUBSTATE(Shutdown, Connected) {
		struct Box {
		};
		STATE_C(Shutdown)

		void evSubSysPowered()
		{
			setState<Initializing>();		
		}

	private:
		// Entry and exit actions of state
		void init();		
		void entry(){}
		void exit(){}
	};

	Macho::Machine<Top> *m;
	SVCID ss;// subsystem service id
	SVCID eh;// error handling service id


	/**Operations needed for subsysterm control service thread init
	 */
	virtual int scThreadInit(){return 0;};

	void cbEvSubSysPowered(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysPowered();
	}
	
	void cbEvSubSysConnecteded(EV_ECL_CONNECTED_TYPE *event)
	{
	    if((((*m)->box().ss == event->to) && ((*m)->box().sid == event->from)) ||
	    		(((*m)->box().ss == event->from) && ((*m)->box().sid == event->to) )){
            (*m)->evSubSysConnecteded();}
	}
	
	void cbEvSubSysDisConnecteded(EV_ECL_DISCONNECTED_TYPE *event)
	{
		if((((*m)->box().ss == event->to) && ((*m)->box().sid == event->from)) ||
			    		(((*m)->box().ss == event->from) && ((*m)->box().sid == event->to) )){
            (*m)->evSubSysDisConnecteded();}
	}
	
	void cbEvSubSysReady(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysReady();
	}

	void cbEvSubSysEnterFault(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysEnterFault();
	}
	void cbEvSubSysLeaveFault(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysLeaveFault();
	}
	void cbEvSubSysEnterShutdown(EVENT_HEADER_TYPE *event)
	{
		(*m)->evSubSysEnterShutdown();
	}

	void cbEvErrorReporting(EV_ERROR_REPORTING_TYPE *event)
	{
		//forward the error message to error service
		ECService::sendEvent(eh,EV_ERROR_REPORTING,(void*)event,sizeof(EV_ERROR_REPORTING_TYPE));
		(*m)->evErrorReporting(event);
	}

	void cbSVCFaultRecover()
	{
		//forward fault recover message to subsystem
		ECService::sendEvent((*m)->box().ss, EV_SS_FAULT_RECOVER);
		(*m)->evSVCFaultRecover();
	}

	void cbEvRportXmlVer(EV_XML_VERSION_TYPE *event)
	{

		int xmlSize=0;
		xmlSize=event->header.length-sizeof(event->header);
		event->data[xmlSize] = 0;
		ScGlobal::scDb.updateSoftVer((char *)(event->data));

		if((*m)->box().ss == event->header.sid)
		{
			(*m)->evSubSysVersionResp(event);
		}
	}

	void cbEvSsDumpLog(EV_SS_DUMP_LOG_TO_SVC_TYPE *event)
    {
	    EV_LOG_DUMP_TYPE ev;
        ev.target = event->target;
	    ECService::sendEvent((*m)->box().ss, EV_LOG_DUMP, &ev, sizeof(ev));
    }

	void cbEvSsShutdown(EVENT_HEADER_TYPE *event)
	{
		ECService::sendEvent((*m)->box().ss, EV_SS_SHUTDOWN);
	}

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
		registerEvent(EV_SS_POWERED,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysPowered);
		registerEvent(EV_ECL_CONNECTED,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysConnecteded);
		registerEvent(EV_ECL_DISCONNECTED,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysDisConnecteded);
		registerEvent(EV_SS_READY,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysReady);
		registerEvent(EV_SS_ENTER_FAULT,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysEnterFault);
		registerEvent(EV_SS_LEAVE_FAULT,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysLeaveFault);
		registerEvent(EV_SS_ENTER_SHUTDOWN,(EventHandler)&SubSysCtrlTemplate::cbEvSubSysEnterShutdown);
		registerEvent(EV_ERROR_REPORTING,(EventHandler)&SubSysCtrlTemplate::cbEvErrorReporting);
		registerEvent(EV_SVC_SC_FAULT_RECOVER, (EventHandler)&SubSysCtrlTemplate::cbSVCFaultRecover);
		registerEvent(EV_XML_VERSION,(EventHandler)&SubSysCtrlTemplate::cbEvRportXmlVer);
		registerEvent(EV_SS_DUMP_LOG_TO_SVC,(EventHandler)&SubSysCtrlTemplate::cbEvSsDumpLog);
		registerEvent(EV_SS_SHUTDOWN,(EventHandler)&SubSysCtrlTemplate::cbEvSsShutdown);
		
		scThreadInit();
		(*m)->start();
		return 0;
	}
};


#endif



