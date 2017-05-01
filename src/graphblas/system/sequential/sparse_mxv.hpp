/*
 * Copyright (c) 2017 Carnegie Mellon University.
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," WITH NO WARRANTIES WHATSOEVER. CARNEGIE
 * MELLON UNIVERSITY EXPRESSLY DISCLAIMS TO THE FULLEST EXTENT PERMITTED BY
 * LAW ALL EXPRESS, IMPLIED, AND STATUTORY WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT OF PROPRIETARY RIGHTS.
 *
 * This Program is distributed under a BSD license.  Please see LICENSE file or
 * permission@sei.cmu.edu for more information.  DM-0002659
 */

/**
 * Implementations of all GraphBLAS functions optimized for the sequential
 * (CPU) backend.
 */

#ifndef GB_SEQUENTIAL_SPARSE_MXV_HPP
#define GB_SEQUENTIAL_SPARSE_MXV_HPP

#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <iterator>
#include <iostream>
#include <graphblas/accum.hpp>
#include <graphblas/algebra.hpp>

#include "sparse_helpers.hpp"


//****************************************************************************

namespace GraphBLAS
{
    namespace backend
    {
        //**********************************************************************
        /// Matrix-vector multiply for LilSparseMatrix and SparseBitmapVector
        template<typename WVectorT,
                 typename MaskT,
                 typename AccumT,
                 typename SemiringT,
                 typename AMatrixT,
                 typename UVectorT>
        inline void mxv(WVectorT        &w,
                        MaskT     const &mask,
                        AccumT           accum,
                        SemiringT        op,
                        AMatrixT  const &A,
                        UVectorT  const &u,
                        bool             replace_flag = false)
        {
            check_vector_size(w, mask,
                              "mxv: failed size(w) == size(mask) check");
            check_vector_size_nrows(w, A,
                                    "mxv: failed size(w) == nrows(A) check");
            check_vector_size_ncols(u, A,
                                    "mxv: failed size(u) == ncols(A) check");

            // =================================================================
            // Do the basic dot-product work with the semi-ring.
            typedef typename SemiringT::result_type D3ScalarType;
            typedef typename AMatrixT::ScalarType AScalarType;
            typedef std::vector<std::tuple<IndexType,AScalarType> >  ARowType;

            std::vector<std::tuple<IndexType, D3ScalarType> > t;

            if ((A.nvals() > 0) && (u.nvals() > 0))
            {
                auto u_contents(u.getContents());
                for (IndexType row_idx = 0; row_idx < w.size(); ++row_idx)
                {
                    ARowType const &A_row(A.getRow(row_idx));

                    if (!A_row.empty())
                    {
                        D3ScalarType t_val;
                        if (dot(t_val, A_row, u_contents, op))
                        {
                            t.push_back(std::make_tuple(row_idx, t_val));
                        }
                    }
                }
            }

            // =================================================================
            // Accumulate into Z
            /// @todo Do we need a type generator for z: D(w) if no accum,
            /// or D3(accum). I think that output type should be equivalent, but
            /// still need to work the proof.
            typedef typename WVectorT::ScalarType WScalarType;
            std::vector<std::tuple<IndexType, WScalarType> > z;
            ewise_or_opt_accum_1D(z, w, t, accum);

            // =================================================================
            // Copy Z into the final output, w, considering mask and replace
            write_with_opt_mask_1D(w, z, mask, replace_flag);
        }

    } // backend
} // GraphBLAS

#endif
