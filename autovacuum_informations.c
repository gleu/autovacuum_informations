/*-------------------------------------------------------------------------
 *
 * autovacuum_informations.c
 *
 *
 * Copyright (c) 2015, Guillaume Lelarge
 *
 * Author: Guillaume Lelarge <guillaume@lelarge.info>
 *
 * IDENTIFICATION
 *	  autovacuum_informations.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "access/htup_details.h"
#include "commands/vacuum.h"
#include "funcapi.h"
#include "lib/ilist.h"
#include "pgstat.h"
#include "storage/proc.h"
#include "autovacuum_informations.h"
#include "utils/builtins.h"
#include "utils/timeout.h"
#include "utils/timestamp.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(get_autovacuum_launcher_pid);
PG_FUNCTION_INFO_V1(get_autovacuum_workers_infos);

/*
typedef struct
{
	char	   *location;
	DIR		   *dirdesc;
} directory_fctx;
*/

Datum
get_autovacuum_launcher_pid(PG_FUNCTION_ARGS)
{
	int pid = -1;
    bool found;

    AutoVacuumShmemStruct *AutoVacuumShmem = (AutoVacuumShmemStruct *)
        ShmemInitStruct("AutoVacuum Data",
        AutoVacuumShmemSize(),
        &found);

	if (found && AutoVacuumShmem)
	{
		LWLockAcquire(AutovacuumLock, LW_EXCLUSIVE);
		pid = AutoVacuumShmem->av_launcherpid;
		LWLockRelease(AutovacuumLock);
		if (pid > 0)
			PG_RETURN_INT64(pid);
	}

	PG_RETURN_NULL();
}

Datum
get_autovacuum_workers_infos(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	dlist_node *cur_node;
    bool found;
    int idx = 1;

    AutoVacuumShmemStruct *AutoVacuumShmem = (AutoVacuumShmemStruct *)
        ShmemInitStruct("AutoVacuum Data",
        AutoVacuumShmemSize(),
        &found);

	if (found && AutoVacuumShmem)
	{
		if (SRF_IS_FIRSTCALL())
		{
			MemoryContext oldcontext;
			TupleDesc	tupdesc;

			funcctx = SRF_FIRSTCALL_INIT();
			oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

			tupdesc = CreateTemplateTupleDesc(9, false);
			TupleDescInitEntry(tupdesc, (AttrNumber) 1, "idx", INT4OID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 2, "datid", OIDOID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 3, "relid", OIDOID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 4, "pid", INT4OID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 5, "launchtime", TIMESTAMPTZOID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 6, "dobalance", BOOLOID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 7, "cost_delay", INT4OID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 8, "cost_limit", INT4OID, -1, 0);
			TupleDescInitEntry(tupdesc, (AttrNumber) 9, "cost_limit_base", INT4OID, -1, 0);

			funcctx->tuple_desc = BlessTupleDesc(tupdesc);

			MemoryContextSwitchTo(oldcontext);
		}

		funcctx = SRF_PERCALL_SETUP();

		cur_node = dlist_head_node(&AutoVacuumShmem->av_runningWorkers);
		for (idx=0; idx<funcctx->call_cntr; idx++)
		{
			if (dlist_has_next(&AutoVacuumShmem->av_runningWorkers, cur_node))
				cur_node = dlist_next_node(&AutoVacuumShmem->av_runningWorkers, cur_node);
			else
				cur_node = NULL;
		}

		if (cur_node)
		{
			Datum	    values[9];
			bool        nulls[9];
			HeapTuple	tuple;
			WorkerInfo	worker = dlist_container(WorkerInfoData, wi_links, cur_node);

	        /*
	         * Form tuple with default data.
	         */
	        MemSet(values, 0, sizeof(values));
	        MemSet(nulls, false, sizeof(nulls));

	        /*
	         * Form tuple with actual data.
	         */
			values[0] = Int32GetDatum(idx);
			values[1] = Int32GetDatum(worker->wi_dboid);
			values[2] = Int32GetDatum(worker->wi_tableoid);
			values[3] = Int32GetDatum(worker->wi_proc->pid);
			values[4] = TimestampTzGetDatum(worker->wi_launchtime);
			values[5] = BoolGetDatum(worker->wi_dobalance);
			values[6] = Int32GetDatum(worker->wi_cost_delay);
			values[7] = Int32GetDatum(worker->wi_cost_limit);
			values[8] = Int32GetDatum(worker->wi_cost_limit_base);

	        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

			SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
		}
	}

	funcctx = SRF_PERCALL_SETUP();
	SRF_RETURN_DONE(funcctx);
}
