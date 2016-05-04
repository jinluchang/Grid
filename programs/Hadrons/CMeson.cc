/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/CMeson.cc

Copyright (C) 2015

Author: Antonin Portelli <antonin.portelli@me.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

See the full license in the file "LICENSE" in the top level distribution 
directory.
*******************************************************************************/

#include <Hadrons/CMeson.hpp>

using namespace Grid;
using namespace QCD;
using namespace Hadrons;

/******************************************************************************
 *                          CMeson implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
CMeson::CMeson(const std::string name)
: Module(name)
{}

// parse parameters ////////////////////////////////////////////////////////////
void CMeson::parseParameters(XmlReader &reader, const std::string name)
{
    read(reader, name, par_);
}

// dependencies/products ///////////////////////////////////////////////////////
std::vector<std::string> CMeson::getInput(void)
{
    std::vector<std::string> input = {par_.q1, par_.q2};
    
    return input;
}

std::vector<std::string> CMeson::getOutput(void)
{
    std::vector<std::string> output = {getName()};
    
    return output;
}

// execution ///////////////////////////////////////////////////////////////////
void CMeson::execute(Environment &env)
{
    LOG(Message) << "Computing meson contraction '" << getName() << "' using"
                 << " quarks '" << par_.q1 << " and '" << par_.q2 << "'"
                 << std::endl;
    
    XmlWriter             writer(par_.output);
    LatticePropagator     &q1 = *env.get<LatticePropagator>(par_.q1);
    LatticePropagator     &q2 = *env.get<LatticePropagator>(par_.q2);
    LatticeComplex        c(env.getGrid());
    SpinMatrix            g[Ns*Ns], g5;
    std::vector<TComplex> buf;
    Result                result;
    unsigned int          nt = env.getGrid()->GlobalDimensions()[Tp];
    
    g5 = makeGammaProd(Ns*Ns - 1);
    result.corr.resize(Ns*Ns);
    for (unsigned int i = 0; i < Ns*Ns; ++i)
    {
        g[i] = makeGammaProd(i);
    }
    for (unsigned int iSink = 0; iSink < Ns*Ns; ++iSink)
    {
        result.corr[iSink].resize(Ns*Ns);
        for (unsigned int iSrc = 0; iSrc < Ns*Ns; ++iSrc)
        {
            c = trace(g[iSink]*q1*g[iSrc]*g5*adj(q2)*g5);
            sliceSum(c, buf, Tp);
            result.corr[iSink][iSrc].resize(buf.size());
            for (unsigned int t = 0; t < buf.size(); ++t)
            {
                result.corr[iSink][iSrc][t] = TensorRemove(buf[t]);
            }
        }
    }
    write(writer, "meson", result);
}
