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

Class
    Foam::functionEntries::codeStream

Description
    Dictionary entry that contains C++ OpenFOAM code that is compiled to
    generate the entry itself. So
    - codeStream reads three entries: 'code', 'codeInclude' (optional),
    'codeOptions' (optional)
    and uses those to generate library sources inside \f codeStream/
    - these get compiled using 'wmake libso'
    - the resulting library is loaded in executed with as arguments
    \code
        (const dictionary& dict, Ostream& os)
     \endcode
    where the dictionary is the current dictionary.
    - the code has to write into Ostream which is then used to construct
    the actual dictionary entry.


    E.g. to set the internal field of a field:

    \verbatim
    internalField  #codeStream
    {
        code
        #{
            const IOdictionary& d = static_cast<const IOdictionary&>(dict);
            const fvMesh& mesh = refCast<const fvMesh>(d.db());
            scalarField fld(mesh.nCells(), 12.34);
            fld.writeEntry("", os);
        #};

        //- Optional:
        codeInclude
        #{
            #include "fvCFD.H"
        #};

        //- Optional:
        codeOptions
        #{
            -I$(LIB_SRC)/finiteVolume/lnInclude
        #};
    };
    \endverbatim


    Note the \c \#{ ... \c \#} syntax is a 'verbatim' input mode that allows
    inputting strings with embedded newlines.

    Limitations:
    - '~' symbol not allowed inside the code sections.
    - probably some other limitations (uses string::expand which expands
      \c \$ and \c ~ sequences)

Note
    The code to be compiled is stored under the local \f codeStream directory
    with a subdirectory name corresponding to the SHA1 of the contents.

    The corresponding library code is located under the local
    \f codeStream/platforms/$WM_OPTIONS/lib directory in a library
    \f libcodeStream_SHA1.so

SourceFiles
    codeStream.C

\*---------------------------------------------------------------------------*/

#ifndef codeStream_H
#define codeStream_H

#include "functionEntry.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
class dlLibraryTable;

namespace functionEntries
{

/*---------------------------------------------------------------------------*\
                         Class codeStream Declaration
\*---------------------------------------------------------------------------*/

class codeStream
:
    public functionEntry
{

        //- Interpreter function type
        typedef void (*streamingFunctionType)(Ostream&, const dictionary&);

    // Private Member Functions

        //- Helper function: parent (of parent etc.) of dictionary up to the top
        static const dictionary& topDict(const dictionary&);

        //- Helper function: access to dlLibraryTable of Time
        static dlLibraryTable& libs(const dictionary& dict);

        //- Disallow default bitwise copy construct
        codeStream(const codeStream&);

        //- Disallow default bitwise assignment
        void operator=(const codeStream&);


public:

    // Static data members

        //- Name of the C code template to be used
        static const word codeTemplateC;


    //- Runtime type information
    ClassName("codeStream");


    // Member Functions

        static bool execute
        (
            const dictionary& parentDict,
            primitiveEntry&,
            Istream&
        );

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace functionEntries
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
