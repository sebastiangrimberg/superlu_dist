#include "superlu_ddefs.h"

static void matCopy(int n, int m, double* Dst, int lddst
    , double* Src, int ldsrc)
{
    for(int j=0; j<m;j++)
        for (int i = 0; i < n; ++i)
        {
            Dst[i+ lddst*j] = Src[i+ ldsrc*j];
        }

        return;
}
// typedef struct {
//     int_t nnz_loc;   /* number of nonzeros in the local submatrix */
//     int_t m_loc;     /* number of rows local to this processor */
//     int_t fst_row;   /* global index of the first row */
//     void  *nzval;    /* pointer to array of nonzero values, packed by row */
//     int_t *rowptr;   /* pointer to array of beginning of rows in nzval[]
// 			and colind[]  */
//     int_t *colind;   /* pointer to array of column indices of the nonzeros */
//                      /* Note:
// 			Zero-based indexing is used;
// 			rowptr[] has n_loc + 1 entries, the last one pointing
// 			beyond the last row, so that rowptr[n_loc] = nnz_loc.*/
// } NRformat_loc;

NRformat_loc dGatherNRformat_loc(NRformat_loc *A, 
    double* B, int ldb, int nrhs, double** B2d,
    gridinfo3d_t *grid3d)
{
    NRformat_loc A2d;

    // find number of nnzs
    int_t *nnz_counts, *row_counts;
    int *nnz_disp, *row_disp, *nnz_counts_int, *row_counts_int;
    int *b_counts_int, *b_disp; 
    nnz_counts = SUPERLU_MALLOC(grid3d->npdep * sizeof(int_t));
    row_counts = SUPERLU_MALLOC(grid3d->npdep * sizeof(int_t));
    nnz_counts_int = SUPERLU_MALLOC(grid3d->npdep * sizeof(int));
    row_counts_int = SUPERLU_MALLOC(grid3d->npdep * sizeof(int));
    b_counts_int = SUPERLU_MALLOC(grid3d->npdep * sizeof(int));
    MPI_Gather(&A->nnz_loc, 1, mpi_int_t, nnz_counts,
               1, mpi_int_t, 0, grid3d->zscp.comm);
    MPI_Gather(&A->m_loc, 1, mpi_int_t, row_counts,
               1, mpi_int_t, 0, grid3d->zscp.comm);
    nnz_disp = SUPERLU_MALLOC((grid3d->npdep + 1) * sizeof(int));
    row_disp = SUPERLU_MALLOC((grid3d->npdep + 1) * sizeof(int));
    b_disp = SUPERLU_MALLOC((grid3d->npdep + 1) * sizeof(int));

    nnz_disp[0] = 0;
    row_disp[0] = 0;
    b_disp[0] =0;
    for (int i = 0; i < grid3d->npdep; i++)
    {
        nnz_disp[i + 1] = nnz_disp[i] + nnz_counts[i];
        row_disp[i + 1] = row_disp[i] + row_counts[i];
        b_disp[i+1] = nrhs*row_disp[i + 1];
        nnz_counts_int[i] = nnz_counts[i];
        row_counts_int[i] = row_counts[i];
        b_counts_int[i] = nrhs*row_counts[i];
    }

    if (grid3d->zscp.Iam == 0)
    {
        A2d.colind = SUPERLU_MALLOC(nnz_disp[grid3d->npdep] * sizeof(int_t));
        A2d.nzval = SUPERLU_MALLOC(nnz_disp[grid3d->npdep] * sizeof(double));
        A2d.rowptr = SUPERLU_MALLOC((row_disp[grid3d->npdep] + 1) * sizeof(int_t));
        A2d.rowptr[0] = 0;
    }

    MPI_Gatherv(A->nzval, A->nnz_loc, MPI_DOUBLE, A2d.nzval,
                nnz_counts_int, nnz_disp,
                MPI_DOUBLE, 0, grid3d->zscp.comm);
    MPI_Gatherv(A->colind, A->nnz_loc, mpi_int_t, A2d.colind,
                nnz_counts_int, nnz_disp,
                mpi_int_t, 0, grid3d->zscp.comm);

    MPI_Gatherv(&A->rowptr[1], A->m_loc, mpi_int_t, &A2d.rowptr[1],
                row_counts_int, row_disp,
                mpi_int_t, 0, grid3d->zscp.comm);

    if (grid3d->zscp.Iam == 0)
    {
        for (int i = 0; i < grid3d->npdep; i++)
        {
            for(int j = row_disp[i]+1; j<row_disp[i+1]+1; j++)
            {
                // A2d.rowptr[j] += row_disp[i];
                A2d.rowptr[j] +=nnz_disp[i];
            }
        }   
        A2d.nnz_loc = nnz_disp[grid3d->npdep];
        A2d.m_loc = row_disp[grid3d->npdep];
        A2d.fst_row = A->fst_row;
    }
    
    // compacting B
    double *Btmp =SUPERLU_MALLOC(A->m_loc*nrhs * sizeof(int_t));
    matCopy(A->m_loc, nrhs,  Btmp, A->m_loc,B, ldb); 

    double *B1;
    if (grid3d->zscp.Iam == 0)
    {
        B1 = SUPERLU_MALLOC( A2d.m_loc*nrhs * sizeof(double));
        *B2d  = SUPERLU_MALLOC( A2d.m_loc*nrhs * sizeof(double));
    }

    MPI_Gatherv(Btmp, nrhs*A->m_loc, MPI_DOUBLE, B1,
                b_counts_int, b_disp,
                MPI_DOUBLE, 0, grid3d->zscp.comm);
    if (grid3d->zscp.Iam == 0)
    {
        for (int i = 0; i < grid3d->npdep; ++i)
        {
            /* code */
            matCopy(row_counts_int[i], nrhs, *B2d + row_disp[i], A2d.m_loc, 
                B1+nrhs*row_disp[i], row_counts_int[i]);
        }

        SUPERLU_FREE(B1);
    }



    return A2d;
}