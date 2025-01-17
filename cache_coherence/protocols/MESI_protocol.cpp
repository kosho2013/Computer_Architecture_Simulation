#include "MESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    // initialize
    this->state = MESI_CACHE_I;
}

MESI_protocol::~MESI_protocol ()
{
}

void MESI_protocol::dump (void)
{
    const char *block_states[8] = {"X","I","S","M","E","ISE","IM","SM"};
    fprintf (stderr, "MESI_protocol - state: %s\n", block_states[state]);
}

void MESI_protocol::process_cache_request (Mreq *request)
{
    switch (state) { // switch on states
	    case MESI_CACHE_I: do_cache_I (request); break;
        case MESI_CACHE_S: do_cache_S (request); break;
        case MESI_CACHE_M: do_cache_M (request); break;
        case MESI_CACHE_E: do_cache_E (request); break;
	    case MESI_CACHE_ISE: do_cache_ISE (request); break;
	    case MESI_CACHE_IM: do_cache_IM (request); break;
	    case MESI_CACHE_SM: do_cache_SM (request); break;

        default:
            fatal_error ("MESI_protocol->state not valid?\n");
    }
}

void MESI_protocol::process_snoop_request (Mreq *request)
{
    switch (state) { // switch on states
        case MESI_CACHE_I: do_snoop_I (request); break;
        case MESI_CACHE_S: do_snoop_S (request); break;
        case MESI_CACHE_M: do_snoop_M (request); break;
        case MESI_CACHE_E: do_snoop_E (request); break;
        case MESI_CACHE_ISE: do_snoop_ISE (request); break;
        case MESI_CACHE_IM: do_snoop_IM (request); break;
        case MESI_CACHE_SM: do_snoop_SM (request); break;

        default:
            fatal_error ("MESI_protocol->state not valid?\n");
    }
}

inline void MESI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) { // read, upgrade to S, write, upgrade to M
        case LOAD:
            send_GETS(request->addr);
            state = MESI_CACHE_ISE;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MESI_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) { // read, send data, write, upgrade to M
        case LOAD:
		    send_DATA_to_proc(request->addr);
    	    break;
        case STORE:
		    send_GETM(request->addr);
		    state = MESI_CACHE_SM;
		    Sim->cache_misses++; // coherence miss
    	    break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg) { // read, send data, write, send data
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg) { // read, send data, write, send data, silent upgrade to E
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
	    case STORE:
            send_DATA_to_proc(request->addr);
            state = MESI_CACHE_M; // silent upgrade
            Sim->silent_upgrades++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_ISE (Mreq *request)
{
    switch (request->msg) { // don't do anything
        case LOAD:
            break;
	    case STORE:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: ISE state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_IM (Mreq *request)
{
    switch (request->msg) { // don't do anything
        case LOAD:
            break;
	    case STORE:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_SM (Mreq *request)
{
    switch (request->msg) { // don't do anything
        case LOAD:
            break;
        case STORE:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}









inline void MESI_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg) { // don't do anything
        case GETS:
            break;
        case GETM:
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, GETM, invalidate
        case GETS:
		    set_shared_line();
		    break;
        case GETM:
		    state = MESI_CACHE_I;
		    break;
        case DATA:
    	    break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, send data, downgrade to S, GETM, send data, downgrade to I
        case GETS:
            set_shared_line();
		    send_DATA_on_bus(request->addr, request->src_mid);
            state = MESI_CACHE_S;
		    break;
        case GETM:
		    send_DATA_on_bus(request->addr, request->src_mid);
		    state = MESI_CACHE_I;
		    break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, send data, downgrade to S, GETM, send data, downgrade to I
        case GETS:
            set_shared_line();
		    send_DATA_on_bus(request->addr, request->src_mid);
            state = MESI_CACHE_S;
		    break;
        case GETM:
		    send_DATA_on_bus(request->addr, request->src_mid);
		    state = MESI_CACHE_I;
		    break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_ISE (Mreq *request)
{
    switch (request->msg) { // data, send data, upgrade to E or S
        case GETS:
            break;
        case GETM:
		    break;
        case DATA:
		    send_DATA_to_proc(request->addr);
            if (get_shared_line()) { // more than 1 copy
                state = MESI_CACHE_S;
            } else { // 1 copy
                state = MESI_CACHE_E;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: ISE state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg) { // data, send data, upgrade to M
        case GETS:
            break;
        case GETM:
		    break;
        case DATA:
		    send_DATA_to_proc(request->addr);
            state = MESI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, GETM, downgrade to IM, data, send data, upgrade to M
        case GETS:
            set_shared_line();
            break;
        case GETM:
            state = MESI_CACHE_IM;
            break;
        case DATA:
    		send_DATA_to_proc(request->addr);
            state = MESI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}
