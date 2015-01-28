/*  Copyright (C) 2014 CZ.NIC, z.s.p.o. <knot-dns@labs.nic.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <libknot/dname.h>
#include <libknot/internal/lists.h>
#include <libknot/internal/namedb/namedb.h>
#include <libknot/internal/sockaddr.h>

#include "lib/context.h"
#include "lib/zonecut.h"

/* Query flags */
enum {
	QUERY_NO_MINIMIZE = 1 << 0, /*!< Don't minimize QNAME. */
	QUERY_TCP         = 1 << 1  /*!< Use TCP for this query. */
};

/*!
 * \brief Single query representation.
 */
struct kr_query {
	node_t node;
	struct kr_query *parent;
	struct kr_zonecut zone_cut;
	struct timeval timestamp;
	knot_dname_t *sname;
	uint16_t stype;
	uint16_t sclass;
	uint16_t id;
	uint16_t flags;
};

/*!
 * \brief Query resolution plan structure.
 *
 * The structure most importantly holds the original query, answer and the
 * list of pending queries required to resolve the original query.
 * It also keeps a notion of current zone cut.
 */
struct kr_rplan {
	unsigned txn_flags;          /*!< Current transaction flags. */
	namedb_txn_t txn;            /*!< Current transaction (may be r/o). */
	list_t pending;              /*!< List of pending queries. */
	list_t resolved;             /*!< List of resolved queries. */
	struct kr_context *context;  /*!< Parent resolution context. */
	mm_ctx_t *pool;              /*!< Temporary memory pool. */
};

/*!
 * \brief Initialize resolution plan (empty).
 * \param rplan plan instance
 * \param context resolution context
 * \param pool ephemeral memory pool for whole resolution
 */
void kr_rplan_init(struct kr_rplan *rplan, struct kr_context *context, mm_ctx_t *pool);

/*!
 * \brief Deinitialize resolution plan, aborting any uncommited transactions.
 * \param rplan plan instance
 */
void kr_rplan_deinit(struct kr_rplan *rplan);

/*!
 * \brief Return true if the resolution plan is empty (i.e. finished or initialized)
 * \param rplan plan instance
 * \return true or false
 */
bool kr_rplan_empty(struct kr_rplan *rplan);

/*!
 * \brief Acquire rplan transaction (read or write only).
 * \note The transaction is shared during the whole resolution, read only transactions
 *       may be promoted to write-enabled transactions if requested, but never demoted.
 * \param rplan plan instance
 * \param flags transaction flags
 * \return transaction instance or NULL
 */
namedb_txn_t *kr_rplan_txn_acquire(struct kr_rplan *rplan, unsigned flags);

/*!
 * \brief Commit any existing transaction, read-only transactions may be just aborted.
 * \param rplan plan instance
 * \return KNOT_E*
 */
int kr_rplan_txn_commit(struct kr_rplan *rplan);

/*!
 * \brief Push a query to the top of the resolution plan.
 * \note This means that this query takes precedence before all pending queries.
 * \param rplan plan instance
 * \param parent query parent (or NULL)
 * \param name resolved name
 * \param cls  resolved class
 * \param type resolved type
 * \return query instance or NULL
 */
struct kr_query *kr_rplan_push(struct kr_rplan *rplan, struct kr_query *parent,
                               const knot_dname_t *name, uint16_t cls, uint16_t type);

/*!
 * \brief Pop existing query from the resolution plan.
 * \note Popped queries are not discarded, but moved to the resolved list.
 * \param rplan plan instance
 * \param qry resolved query
 * \return KNOT_E*
 */
int kr_rplan_pop(struct kr_rplan *rplan, struct kr_query *qry);

/*!
 * \brief Currently resolved query (at the top).
 * \param rplan plan instance
 * \return query instance or NULL if empty
 */
struct kr_query *kr_rplan_current(struct kr_rplan *rplan);