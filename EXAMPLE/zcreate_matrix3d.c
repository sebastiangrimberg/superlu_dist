/*! \file
Copyright (c) 2003, The Regents of the University of California, through
Lawrence Berkeley National Laboratory (subject to receipt of any required
approvals from U.S. Dept. of Energy)

All rights reserved.

The source code is distributed under BSD license, see the file License.txt
at the top-level directory.
*/


/*! @file
 * \brief Read the matrix from data file
 *
 * <pre>
 * -- Distributed SuperLU routine (version 9.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley,
 * Oak Ridge National Lab.
 * May 12, 2021
 * July 15, 2022
 * </pre>
 */
#include <math.h>
#include "superlu_zdefs.h"

/* \brief
 *
 * <pre>
 * Purpose
 * =======
 *
 * ZCREATE_MATRIX3D_POSTFIX read the matrix from data file with the
 * storage format described by the suffix string "postfix";
 * Distribute it to processors in a distributed compressed row format;
 * Generate the distributed true solution X and the right-hand side RHS.
 *
 *
 * Arguments
 * =========
 *
 * A     (output) SuperMatrix*
 *       Local matrix A in NR_loc format.
 *
 * NRHS  (input) int_t
 *       Number of right-hand sides.
 *
 * RHS   (output) double**
 *       The right-hand side matrix.
 *
 * LDB   (output) int*
 *       Leading dimension of the right-hand side matrix.
 *
 * X     (output) double**
 *       The true solution matrix.
 *
 * LDX   (output) int*
 *       The leading dimension of the true solution matrix.
 *
 * FP    (input) FILE*
 *       The matrix file pointer.
 *
 * postfix (input) char*
 *       Suffix of the input file name.
 *
 * GRID  (input) gridinfo3d_t*
 *       The 3D process mesh.
 *
 * </pre>
 */
int zcreate_matrix_postfix3d(SuperMatrix *A, int nrhs, doublecomplex **rhs,
                           int *ldb, doublecomplex **x, int *ldx,
                           FILE *fp, char * postfix, gridinfo3d_t *grid3d)
{
    SuperMatrix GA;              /* global A */
    doublecomplex   *b_global, *xtrue_global;  /* replicated on all processes */
    int_t    *rowind, *colptr;   /* global */
    doublecomplex   *nzval;             /* global */
    doublecomplex   *nzval_loc;         /* local */
    int_t    *colind, *rowptr;   /* local */
    int_t    m, n, nnz;
    int_t    m_loc, fst_row, nnz_loc;
    int_t    m_loc_fst; /* Record m_loc of the first p-1 processors,
               when mod(m, p) is not zero. */
    int_t    row, col, i, j, relpos;
    int      iam;
    char     trans[1];
    int_t      *marker;

    iam = grid3d->iam;

#if ( DEBUGlevel>=1 )
    CHECK_MALLOC(iam, "Enter zcreate_matrix_postfix3d()");
#endif

    if ( !iam )
    {
        double t = SuperLU_timer_();

        if(!strcmp(postfix,"cua"))
        {
            /* Read the matrix stored on disk in Harwell-Boeing format. */
            zreadhb_dist(iam, fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "mtx"))
        {
            /* Read the matrix stored on disk in Matrix Market format. */
            zreadMM_dist(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "rb"))
        {
            /* Read the matrix stored on disk in Rutherford-Boeing format. */
            zreadrb_dist(iam, fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "dat"))
        {
            /* Read the matrix stored on disk in triplet format. */
            zreadtriple_dist(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "datnh"))
        {
            /* Read the matrix stored on disk in triplet format (without header). */
            zreadtriple_noheader(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "bin"))
        {
            /* Read the matrix stored on disk in binary format. */
            zread_binary(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else
        {
            ABORT("File format not known");
        }

        printf("Time to read and distribute matrix %.2f\n",
               SuperLU_timer_() - t);  fflush(stdout);

        /* Broadcast matrix A to the other PEs. */
        MPI_Bcast( &m,     1,   mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( &n,     1,   mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( &nnz,   1,   mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( nzval,  nnz, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
        MPI_Bcast( rowind, nnz, mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( colptr, n + 1, mpi_int_t,  0, grid3d->comm );
    }
    else
    {
        /* Receive matrix A from PE 0. */
        MPI_Bcast( &m,   1,   mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( &n,   1,   mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( &nnz, 1,   mpi_int_t,  0, grid3d->comm );

        /* Allocate storage for compressed column representation. */
        zallocateA_dist(n, nnz, &nzval, &rowind, &colptr);

        MPI_Bcast( nzval,   nnz, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
        MPI_Bcast( rowind,  nnz, mpi_int_t,  0, grid3d->comm );
        MPI_Bcast( colptr,  n + 1, mpi_int_t,  0, grid3d->comm );
    }

#if 0
    nzval[0] = 0.1;
#endif

    /* Compute the number of rows to be distributed to local process */
    m_loc = m / (grid3d->nprow * grid3d->npcol* grid3d->npdep);
    m_loc_fst = m_loc;
    /* When m / procs is not an integer */
    if ((m_loc * grid3d->nprow * grid3d->npcol* grid3d->npdep) != m)
    {
        /*m_loc = m_loc+1;
          m_loc_fst = m_loc;*/
        if (iam == (grid3d->nprow * grid3d->npcol* grid3d->npdep - 1)) /* last proc. gets all*/
            m_loc = m - m_loc * (grid3d->nprow * grid3d->npcol* grid3d->npdep - 1);
    }

    /* Create compressed column matrix for GA. */
    zCreate_CompCol_Matrix_dist(&GA, m, n, nnz, nzval, rowind, colptr,
                                SLU_NC, SLU_D, SLU_GE);

    /* Generate the exact solution and compute the right-hand side. */
    if ( !(b_global = doublecomplexMalloc_dist(m * nrhs)) )
        ABORT("Malloc fails for b[]");
    if ( !(xtrue_global = doublecomplexMalloc_dist(n * nrhs)) )
        ABORT("Malloc fails for xtrue[]");
    *trans = 'N';

    if (iam == 0) {
        zGenXtrue_dist(n, nrhs, xtrue_global, n);
        zFillRHS_dist(trans, nrhs, xtrue_global, n, &GA, b_global, m);
        MPI_Bcast( xtrue_global, n*nrhs, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
        MPI_Bcast( b_global, m*nrhs, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
    } else {
        MPI_Bcast( xtrue_global, n*nrhs, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
        MPI_Bcast( b_global, m*nrhs, SuperLU_MPI_DOUBLE_COMPLEX, 0, grid3d->comm );
    }



    /*************************************************
     * Change GA to a local A with NR_loc format     *
     *************************************************/

    rowptr = (int_t *) intMalloc_dist(m_loc + 1);
    marker = (int_t *) intCalloc_dist(n);

    /* Get counts of each row of GA */
    for (i = 0; i < n; ++i)
        for (j = colptr[i]; j < colptr[i + 1]; ++j) ++marker[rowind[j]];
    /* Set up row pointers */
    rowptr[0] = 0;
    fst_row = iam * m_loc_fst;
    nnz_loc = 0;
    for (j = 0; j < m_loc; ++j)
    {
        row = fst_row + j;
        rowptr[j + 1] = rowptr[j] + marker[row];
        marker[j] = rowptr[j];
    }
    nnz_loc = rowptr[m_loc];

    nzval_loc = (doublecomplex *) doublecomplexMalloc_dist(nnz_loc);
    colind = (int_t *) intMalloc_dist(nnz_loc);

    /* Transfer the matrix into the compressed row storage */
    for (i = 0; i < n; ++i)
    {
        for (j = colptr[i]; j < colptr[i + 1]; ++j)
        {
            row = rowind[j];
            if ( (row >= fst_row) && (row < fst_row + m_loc) )
            {
                row = row - fst_row;
                relpos = marker[row];
                colind[relpos] = i;
                nzval_loc[relpos] = nzval[j];
                ++marker[row];
            }
        }
    }

#if ( DEBUGlevel>=2 )
    if ( !iam ) dPrint_CompCol_Matrix_dist(&GA);
#endif

    /* Destroy GA */
    Destroy_CompCol_Matrix_dist(&GA);

    /******************************************************/
    /* Change GA to a local A with NR_loc format */
    /******************************************************/

    /* Set up the local A in NR_loc format */
    zCreate_CompRowLoc_Matrix_dist(A, m, n, nnz_loc, m_loc, fst_row,
                                   nzval_loc, colind, rowptr,
                                   SLU_NR_loc, SLU_Z, SLU_GE);

    /* Get the local B */
    if ( !((*rhs) = doublecomplexMalloc_dist(m_loc * nrhs)) )
        ABORT("Malloc fails for rhs[]");
    for (j = 0; j < nrhs; ++j)
    {
        for (i = 0; i < m_loc; ++i)
        {
            row = fst_row + i;
            (*rhs)[j * m_loc + i] = b_global[j * n + row];
        }
    }
    *ldb = m_loc;

    /* Set the true X */
    *ldx = m_loc;
    if ( !((*x) = doublecomplexMalloc_dist(*ldx * nrhs)) )
        ABORT("Malloc fails for x_loc[]");

    /* Get the local part of xtrue_global */
    for (j = 0; j < nrhs; ++j)
    {
        for (i = 0; i < m_loc; ++i)
            (*x)[i + j * (*ldx)] = xtrue_global[i + fst_row + j * n];
    }

    SUPERLU_FREE(b_global);
    SUPERLU_FREE(xtrue_global);
    SUPERLU_FREE(marker);

#if ( DEBUGlevel>=1 )
    printf("sizeof(NRforamt_loc) %lu\n", sizeof(NRformat_loc));
    CHECK_MALLOC(iam, "Exit dcreate_matrix()");
#endif
    return 0;
}


/*
 * This routine mimics the batch setup.
 * It reads a sparse matrix from a file, then creates *batchCount* copies,
 * and assign a handle pointing to a copy.
 */
int zcreate_batch_systems(handle_t *SparseMatrix_handles, int batchCount,
			  int nrhs, doublecomplex **RHSptr,
			  int *ldRHS, doublecomplex **xtrue, int *ldX,
			  FILE *fp, char * postfix, gridinfo3d_t *grid3d)
{
    int_t    *rowind, *colptr;   /* global */
    int_t    *rowind_d, *colptr_d;  /* metadata for one diagonal block */
    doublecomplex   *nzval, *nzval_d;      /* global */
    int_t    *colind, *rowptr;   /* local */
    int_t    m, n, nnz;
    int    row, col, i, j, relpos;
    int      iam;
    char     trans[1];

    iam = grid3d->iam;

#if ( DEBUGlevel>=1 )
    CHECK_MALLOC(iam, "Enter zcreate_batch_systems()");
#endif

    if ( !iam )
    {
        double t = SuperLU_timer_();

        if(!strcmp(postfix,"cua"))
        {
            /* Read the matrix stored on disk in Harwell-Boeing format. */
            zreadhb_dist(iam, fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "mtx"))
        {
            /* Read the matrix stored on disk in Matrix Market format. */
            zreadMM_dist(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "rb"))
        {
            /* Read the matrix stored on disk in Rutherford-Boeing format. */
            zreadrb_dist(iam, fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "dat"))
        {
            /* Read the matrix stored on disk in triplet format. */
            zreadtriple_dist(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "datnh"))
        {
            /* Read the matrix stored on disk in triplet format (without header). */
            zreadtriple_noheader(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else if (!strcmp(postfix, "bin"))
        {
            /* Read the matrix stored on disk in binary format. */
            zread_binary(fp, &m, &n, &nnz, &nzval, &rowind, &colptr);
        }
        else
        {
            ABORT("File format not known");
        }

        printf("Time to read and distribute matrix %.2f\n",
               SuperLU_timer_() - t);  fflush(stdout);
    }
	
    /* Allocate storage for CSC containing all the matrices */
    SuperMatrix **A = SUPERLU_MALLOC( batchCount * sizeof(SuperMatrix *) );
    int d = 0;
    for (d = 0; d < batchCount; ++d) {
	int_t  *rowind_d, *colptr_d; /* each block */
	
        /* Allocate storage for compressed column representation. */
        zallocateA_dist(n, nnz, &nzval_d, &rowind_d, &colptr_d);
	
	/* Copy the CSC arrays */
	for (j = 0; j < n+1; ++j) colptr_d[j] = colptr[j];
	for (i = 0; i < nnz; ++i) {
	    rowind_d[i] = rowind[i];
	    nzval_d[i] = nzval[i];
	}
	    
	/* Create compressed column matrix. */
	A[d] = (SuperMatrix *) SUPERLU_MALLOC( sizeof(SuperMatrix) );
	zCreate_CompCol_Matrix_dist(A[d], m, n, nnz, nzval_d, rowind_d, colptr_d,
				    SLU_NC, SLU_Z, SLU_GE);
	SparseMatrix_handles[d] = (handle_t) A[d];
	
	/* Generate the exact solutions and compute the right-hand sides. */
	RHSptr[d] = doublecomplexMalloc_dist( m * nrhs );
	xtrue[d] = doublecomplexMalloc_dist( n * nrhs );
	ldRHS[d] = m;
	ldX[d] = n;
	*trans = 'N';
	zGenXtrue_dist(n, nrhs, xtrue[d], n);
	zFillRHS_dist(trans, nrhs, xtrue[d], n, A[d], RHSptr[d], m);
    }

#if ( DEBUGlevel>=1 )
    CHECK_MALLOC(iam, "Exit zcreate_batch_systems()");
#endif
    return 0;
} /* end zcreate_batch_systems */
