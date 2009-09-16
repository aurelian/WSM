<?php

/* $Id: functions.inc,v 1.1 2005/08/14 16:45:23 aurelian Exp $ */

    
function status($status) {
    switch($status){
        case SERVICE_CONTINUE_PENDING:
            return "The service continue is pending.";
        case SERVICE_PAUSE_PENDING:
            return "The service pause is pending.";
        case SERVICE_PAUSED:
            return "The service is paused.";
        case SERVICE_RUNNING:
            return "The service is running.";
        case SERVICE_START_PENDING:
            return "The service is starting.";
        case SERVICE_STOP_PENDING:
            return "The service is stopping.";
        case SERVICE_STOPPED:
            return "The service is not running.";
        default:
            return "Unknow Status";
    }
}

