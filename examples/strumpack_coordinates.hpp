// Coordinates of the unknowns for STRUMPACK's H2 fronts (--sp_compression h2
// with an ordering other than GEOMETRIC), shared by the examples that solve
// with STRUMPACK (ex1p, ex2p, ex3p with --strumpack).
//
// STRUMPACK options go after a lone "--" on the command line, e.g.
//   ex1p -m ../data/escher.mesh -rs 3 -rp 3 -sp -no-vis -- \
//        --sp_compression h2 --sp_reordering_method metis ...
// SplitStrumpackArgs cuts them off, so that MFEM's OptionsParser only sees its
// own options.

#ifndef MFEM_EXAMPLES_STRUMPACK_COORDINATES_HPP
#define MFEM_EXAMPLES_STRUMPACK_COORDINATES_HPP

#include "mfem.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace mfem
{

// argv up to a lone "--" stays MFEM's (argc is cut there); what follows, after
// argv[0], is STRUMPACK's, in sp_argc / sp_argv (pointers into argv, which
// must outlive the solver: STRUMPACK parses them in SetFromCommandLine).
inline void SplitStrumpackArgs(int &argc, char *argv[],
                               int &sp_argc, std::vector<char *> &sp_argv)
{
   sp_argv.assign(1, argv[0]);
   for (int i = 1; i < argc; i++)
   {
      if (std::strcmp(argv[i], "--") == 0)
      {
         sp_argv.insert(sp_argv.end(), argv + i + 1, argv + argc);
         argc = i;
         break;
      }
   }
   sp_argc = static_cast<int>(sp_argv.size());
   sp_argv.push_back(nullptr);
}

// The coordinates of this rank's true dofs of fes, in the order of the rows of
// the matrices assembled on it, SpaceDimension values each (point-major).  A
// dof is at its interpolation point (the element's FiniteElement::GetNodes
// mapped by the element transformation: vertices, edge and face points for
// H1, edge points for Nedelec), and the components of a vector space are at
// the same point.  A dof shared by elements gets the same point from each.
inline std::vector<double> TrueDofCoordinates(ParFiniteElementSpace &fes)
{
   ParMesh &pmesh = *fes.GetParMesh();
   const int sdim = pmesh.SpaceDimension();
   const int ndofs = fes.GetNDofs();
   std::vector<double> ldof(static_cast<size_t>(ndofs) * sdim, 0.0);
   Array<int> dofs;
   Vector x(sdim);
   for (int e = 0; e < pmesh.GetNE(); e++)
   {
      const FiniteElement &fe = *fes.GetFE(e);
      const IntegrationRule &nodes = fe.GetNodes();
      ElementTransformation &T = *fes.GetElementTransformation(e);
      fes.GetElementDofs(e, dofs);
      MFEM_VERIFY(nodes.GetNPoints() == dofs.Size(),
                  "TrueDofCoordinates: element " << e << " has "
                  << dofs.Size() << " dofs and " << nodes.GetNPoints()
                  << " nodes");
      for (int i = 0; i < dofs.Size(); i++)
      {
         const int d = dofs[i] >= 0 ? dofs[i] : -1 - dofs[i];  // (signed: ND)
         T.Transform(nodes.IntPoint(i), x);
         std::copy(x.GetData(), x.GetData() + sdim,
                   ldof.data() + static_cast<size_t>(d) * sdim);
      }
   }
   std::vector<double> tdof(static_cast<size_t>(fes.GetTrueVSize()) * sdim, 0.0);
   for (int d = 0; d < ndofs; d++)
   {
      for (int c = 0; c < fes.GetVDim(); c++)
      {
         const int t = fes.GetLocalTDofNumber(fes.DofToVDof(d, c));
         if (t >= 0)
         {
            std::copy(ldof.data() + static_cast<size_t>(d) * sdim,
                      ldof.data() + static_cast<size_t>(d + 1) * sdim,
                      tdof.data() + static_cast<size_t>(t) * sdim);
         }
      }
   }
   return tdof;
}

#ifdef MFEM_USE_STRUMPACK
// STRUMPACKSolver that also hands STRUMPACK the coordinates of the unknowns
// (this rank's rows), which its H2 fronts need under a METIS/ParMETIS
// ordering; other compressions ignore them.  The count is given, so the call
// can come before or after SetOperator.
class STRUMPACKCoordinatesSolver : public STRUMPACKSolver
{
public:
   using STRUMPACKSolver::STRUMPACKSolver;

   void SetCoordinates(const std::vector<double> &xyz, int dim)
   {
      solver_->set_coordinates(xyz.data(),
                               static_cast<HYPRE_BigInt>(xyz.size() / dim), dim);
   }
};
#endif

} // namespace mfem

#endif
