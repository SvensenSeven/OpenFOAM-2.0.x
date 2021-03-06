/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    shallowWaterFoam

Description
    Transient solver for inviscid shallow-water equations with rotation.

    If the geometry is 3D then it is assumed to be one layers of cells and the
    component of the velocity normal to gravity is removed.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "pimpleControl.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "readGravitationalAcceleration.H"
    #include "createFields.H"

    pimpleControl pimple(mesh);

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.loop())
    {
        Info<< "\n Time = " << runTime.timeName() << nl << endl;

        #include "CourantNo.H"

        // --- Pressure-velocity PIMPLE corrector loop
        for (pimple.start(); pimple.loop(); pimple++)
        {
            surfaceScalarField phiv("phiv", phi/fvc::interpolate(h));

            fvVectorMatrix hUEqn
            (
                fvm::ddt(hU)
              + fvm::div(phiv, hU)
            );

            hUEqn.relax();

            if (pimple.momentumPredictor())
            {
                if (rotating)
                {
                    solve(hUEqn + (F ^ hU) == -magg*h*fvc::grad(h + h0));
                }
                else
                {
                    solve(hUEqn == -magg*h*fvc::grad(h + h0));
                }

                // Constrain the momentum to be in the geometry if 3D geometry
                if (mesh.nGeometricD() == 3)
                {
                    hU -= (gHat & hU)*gHat;
                    hU.correctBoundaryConditions();
                }
            }

            // --- PISO loop
            for (int corr=0; corr<pimple.nCorr(); corr++)
            {
                volScalarField rAU(1.0/hUEqn.A());
                surfaceScalarField ghrAUf(magg*fvc::interpolate(h*rAU));

                surfaceScalarField phih0(ghrAUf*mesh.magSf()*fvc::snGrad(h0));

                if (rotating)
                {
                    hU = rAU*(hUEqn.H() - (F ^ hU));
                }
                else
                {
                    hU = rAU*hUEqn.H();
                }

                phi = (fvc::interpolate(hU) & mesh.Sf())
                    + fvc::ddtPhiCorr(rAU, h, hU, phi)
                    - phih0;

                for (int nonOrth=0; nonOrth<=pimple.nNonOrthCorr(); nonOrth++)
                {
                    fvScalarMatrix hEqn
                    (
                        fvm::ddt(h)
                      + fvc::div(phi)
                      - fvm::laplacian(ghrAUf, h)
                    );

                    hEqn.solve
                    (
                        mesh.solver
                        (
                            h.select(pimple.finalInnerIter(corr, nonOrth))
                        )
                    );

                    if (nonOrth == pimple.nNonOrthCorr())
                    {
                        phi += hEqn.flux();
                    }
                }

                hU -= rAU*h*magg*fvc::grad(h + h0);

                // Constrain the momentum to be in the geometry if 3D geometry
                if (mesh.nGeometricD() == 3)
                {
                    hU -= (gHat & hU)*gHat;
                }

                hU.correctBoundaryConditions();
            }
        }

        U == hU/h;
        hTotal == h + h0;

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
