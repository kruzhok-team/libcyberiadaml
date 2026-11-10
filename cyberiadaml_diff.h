/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The revision diff C library header
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#ifndef __CYBERIADA_ML_DIFF_H
#define __CYBERIADA_ML_DIFF_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada revision diff codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_DIFF_FLAG_ADDED                         0x1      /* the element is in the second document only */
#define CYBERIADA_DIFF_FLAG_REMOVED                       0x2      /* the element is in the first document only */
#define CYBERIADA_DIFF_FLAG_TYPE                          0x4      /* the type (a simple and a composite state are one) */
#define CYBERIADA_DIFF_FLAG_TITLE                         0x8      /* the title or the formal title; a SM: its name */
#define CYBERIADA_DIFF_FLAG_ACTIONS                       0x10     /* the node actions, see action_flags */
#define CYBERIADA_DIFF_FLAG_PARENT                        0x20     /* the node moved to another parent */
#define CYBERIADA_DIFF_FLAG_LINK                          0x40     /* the submachine reference */
#define CYBERIADA_DIFF_FLAG_COMMENT                       0x80     /* the comment text or markup */
#define CYBERIADA_DIFF_FLAG_COLOR                         0x100    /* the color */
#define CYBERIADA_DIFF_FLAG_GEOMETRY                      0x200    /* the geometry (the geometry option) */
#define CYBERIADA_DIFF_FLAG_COLLAPSED                     0x400    /* the collapsed flag (the geometry option) */
#define CYBERIADA_DIFF_FLAG_SOURCE                        0x800    /* the edge source */
#define CYBERIADA_DIFF_FLAG_TARGET                        0x1000   /* the edge target: a node or the commented edge */
#define CYBERIADA_DIFF_FLAG_TRIGGER                       0x2000   /* the transition trigger */
#define CYBERIADA_DIFF_FLAG_GUARD                         0x4000   /* the transition guard */
#define CYBERIADA_DIFF_FLAG_BEHAVIOR                      0x8000   /* the transition behavior */
#define CYBERIADA_DIFF_FLAG_PROPAGATION                   0x10000  /* the transition event handling keyword */
#define CYBERIADA_DIFF_FLAG_SUBJECT                       0x20000  /* the comment subject type or fragment */
#define CYBERIADA_DIFF_FLAG_META                          0x40000  /* the document metainformation */
#define CYBERIADA_DIFF_FLAG_ELEMENTS                      0x80000  /* a SM: some of its nodes or edges differ */

#define CYBERIADA_DIFF_OPTION_GEOMETRY                    0x1      /* compare the geometry and the collapsed flag */
#define CYBERIADA_DIFF_OPTION_IGNORE_COMMENTS             0x2      /* leave the comments and their links out */

#define CYBERIADA_DIFF_GEOMETRY_TOLERANCE                 0.01     /* the default largest coordinate difference
																	  counted as none */

/* -----------------------------------------------------------------------------
 * The Cyberiada revision diff types
 * ----------------------------------------------------------------------------- */

typedef struct {
	int                 flags;               /* CYBERIADA_DIFF_OPTION_* */
	double              geometry_tolerance;  /* the largest coordinate difference counted as none */
} CyberiadaDiffOptions;

typedef struct {                                        /* a node pair, an added or a removed node */
	CyberiadaNode*      n1;                  /* NULL for an added node */
	CyberiadaNode*      n2;                  /* NULL for a removed node */
	unsigned int        flags;               /* CYBERIADA_DIFF_FLAG_*, 0 for an unchanged pair */
	int                 action_flags;        /* CYBERIADA_ACTION_DIFF_* of the node actions */
} CyberiadaNodeDiff;

typedef struct {                                        /* an edge pair, an added or a removed edge */
	CyberiadaEdge*      e1;                  /* NULL for an added edge */
	CyberiadaEdge*      e2;                  /* NULL for a removed edge */
	unsigned int        flags;               /* CYBERIADA_DIFF_FLAG_*, 0 for an unchanged pair */
} CyberiadaEdgeDiff;

/* The diff of two state machines. The nodes come in the order of the second SM,
   then the removed ones in the order of the first; the edges likewise. */
typedef struct {
	CyberiadaSM*        sm1;                 /* NULL for an added SM */
	CyberiadaSM*        sm2;                 /* NULL for a removed SM */
	unsigned int        flags;               /* of the SM node; ELEMENTS when an element differs */
	size_t              nodes_size;
	CyberiadaNodeDiff*  nodes;
	size_t              edges_size;
	CyberiadaEdgeDiff*  edges;
} CyberiadaSMDiff;

typedef struct {                                        /* a differing metainformation parameter */
	char*               name;
	char*               value1;              /* NULL when the first document lacks it */
	char*               value2;              /* NULL when the second document lacks it */
} CyberiadaMetaDiff;

/* The diff of two documents. The element pointers belong to the compared
   documents, which must outlive the result; the strings belong to the result. */
typedef struct {
	unsigned int        flags;               /* the OR of the SM flags and META; 0: no differences */
	size_t              meta_size;
	CyberiadaMetaDiff*  meta;                /* the differing parameters only */
	size_t              sms_size;
	CyberiadaSMDiff*    sms;                 /* the SMs in the order of the second document,
											    then the removed ones */
} CyberiadaDocumentDiff;

/* -----------------------------------------------------------------------------
 * The Cyberiada revision diff functions
 * ----------------------------------------------------------------------------- */

	/* Fill the default diff options */
	void cyberiada_default_diff_options(CyberiadaDiffOptions* options);

	/* Compare two revisions of a document; NULL options are the defaults */
	/* Note: with the geometry option the documents must share the coordinate formats */
	int cyberiada_diff_documents(CyberiadaDocument* doc1, CyberiadaDocument* doc2,
								 const CyberiadaDiffOptions* options, CyberiadaDocumentDiff* result);

	/* Free the content of the document diff result */
	int cyberiada_cleanup_document_diff(CyberiadaDocumentDiff* result);

	/* Compare two revisions of a state machine; NULL options are the defaults */
	int cyberiada_diff_sms(CyberiadaSM* sm1, CyberiadaSM* sm2,
						   const CyberiadaDiffOptions* options, CyberiadaSMDiff* result);

	/* Free the content of the state machine diff result */
	int cyberiada_cleanup_sm_diff(CyberiadaSMDiff* result);

#ifdef __cplusplus
}
#endif

#endif
