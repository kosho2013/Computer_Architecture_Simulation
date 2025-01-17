#include "MSI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MSI_protocol::MSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    // initialize
    this->state = MSI_CACHE_I;
}

MSI_protocol::~MSI_protocol ()
{
}

void MSI_protocol::dump (void)
{
    const char *block_states[7] = {"X","I","S","M","IM","IS","SM"}; // 3 basic states, 3 intermediate states
    fprintf (stderr, "MSI_protocol - state: %s\n", block_states[state]);
}

void MSI_protocol::process_cache_request (Mreq *request)
{
    switch (state) { // switch on states
	    case MSI_CACHE_I: do_cache_I (request); break;
        case MSI_CACHE_S: do_cache_S (request); break;
        case MSI_CACHE_M: do_cache_M (request); break;
	    case MSI_CACHE_IM: do_cache_IM (request); break;
	    case MSI_CACHE_IS: do_cache_IS (request); break;
	    case MSI_CACHE_SM: do_cache_SM (request); break;

        default:
            fatal_error ("MSI_protocol->state not valid?\n");
    }
}

void MSI_protocol::process_snoop_request (Mreq *request)
{
    switch (state) { // switch on states
        case MSI_CACHE_I: do_snoop_I (request); break;
        case MSI_CACHE_S: do_snoop_S (request); break;
        case MSI_CACHE_M: do_snoop_M (request); break;
        case MSI_CACHE_IM: do_snoop_IM (request); break;
        case MSI_CACHE_IS: do_snoop_IS (request); break;
        case MSI_CACHE_SM: do_snoop_SM (request); break;

        default:
            fatal_error ("MSI_protocol->state not valid?\n");
    }
}

inline void MSI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) { // read, upgrade to S, write, upgrade to M
        case LOAD:
            send_GETS(request->addr);
            state = MSI_CACHE_IS;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MSI_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) { // read, send data, write, upgrade to M
        case LOAD:
		    send_DATA_to_proc(request->addr);
    	    break;
        case STORE:
		    send_GETM(request->addr);
		    state = MSI_CACHE_SM;
		    Sim->cache_misses++; // coherence miss
    	    break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_M (Mreq *request)
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

inline void MSI_protocol::do_cache_IM (Mreq *request)
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

inline void MSI_protocol::do_cache_IS (Mreq *request)
{
    switch (request->msg) { // don't do anything
        case LOAD:
            break;
        case STORE:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_SM (Mreq *request)
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





inline void MSI_protocol::do_snoop_I (Mreq *request)
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

inline void MSI_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, GETM, invalidate
        case GETS:
		    set_shared_line();
		    break;
        case GETM:
		    state = MSI_CACHE_I;
		    break;
        case DATA:
    	    break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, send data, downgrade to S, GETM, send data, downgrade to I
        case GETS:
            set_shared_line();
		    send_DATA_on_bus(request->addr, request->src_mid);
            state = MSI_CACHE_S;
		    break;
        case GETM:
		    send_DATA_on_bus(request->addr, request->src_mid);
		    state = MSI_CACHE_I;
		    break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg) { // data, send data, upgrade to M
        case GETS:
            break;
        case GETM:
		    break;
        case DATA:
		    send_DATA_to_proc(request->addr);
		    state = MSI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg) { // data, send data, upgrade to S
        case GETS:
            break;
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MSI_CACHE_S;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg) { // GETS, set shared, GETM, downgrade to IM, data, send data, upgrade to M
        case GETS:
            set_shared_line();
            break;
        case GETM:
            state = MSI_CACHE_IM;
            break;
        case DATA:
    		send_DATA_to_proc(request->addr);
            state = MSI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}
