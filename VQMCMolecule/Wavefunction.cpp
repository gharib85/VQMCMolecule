#include "Wavefunction.h"

void Wavefunction::Init(const Systems::Molecule& molecule, Random& random, double radiusInit)
{
    // if this would be for a single atom, the selection of orbitals could follow the Madelung rule
    // see the AufbauPrinciple class from the DFT atom project
    
    // being only a toy project
    // will use a simpler method of selecting the basis functions

    // a better implementation would use all orbitals from the basis, generating instead of a Slater determinant,
    // a linear combination of Slater determinants with various basis functions from the last shell changed from one another 
    // perhaps a linear combination of them as it's done for the two atoms molecule
    // the coefficients would be variational parameters to be optimized
    // in that case one would work with better minimum finding methods, too (for some examples, see the DFT Quantum Dot project)

    // but this won't happen with the current project, because it would require rewriting the optimizations and also it would require an insane amount of computation



    // this is particularized for a diatomic molecule 
    // for others, as only atomic orbitals would be used, it won't work very well
        
    // but for now I'll let only the diatomic case, it will be the only one exposed from the UI


    orbitals.clear();

    m_molecule = &molecule;

    unsigned int curParticle = 0;
    unsigned int accumAtomParticles = 0; 

    const unsigned int Ne = molecule.alphaElectrons + molecule.betaElectrons;
    currentParticles.resize(Ne);


    const bool isSpecialMolecule = molecule.atoms.size() == 2;


    // start with an initial random state
    std::vector<Orbitals::ContractedGaussianOrbital> firstAtomOrbsToMerge;
    std::vector<Orbitals::ContractedGaussianOrbital> secondAtomOrbsToMerge;

    for (unsigned int curAtom = 0; curAtom < molecule.atoms.size(); ++curAtom)
    {
        accumAtomParticles += molecule.atoms[curAtom].alphaElectrons;

        for (unsigned int curShell = 0; curShell < molecule.atoms[curAtom].shells.size(); ++curShell)
        {
            const bool isOnLastShell = curShell == molecule.atoms[curAtom].shells.size() - 1;

            for (unsigned int curOrb = 0; curOrb < molecule.atoms[curAtom].shells[curShell].basisFunctions.size(); ++curOrb)
            {
                if (curParticle >= accumAtomParticles)
                    break;

                if (isSpecialMolecule && isOnLastShell)
                {
                    if (curAtom)
                    {
                        if (secondAtomOrbsToMerge.size() <= curOrb) // maybe already added when first atom was considered
                            secondAtomOrbsToMerge.push_back(molecule.atoms[1].shells[curShell].basisFunctions[curOrb]);
                        
                        if (secondAtomOrbsToMerge.size() > firstAtomOrbsToMerge.size())
                        {
                            // try to add an uncompleted orbital from the first atom (last shell, of course), if possible
                            if (molecule.atoms[0].shells[molecule.atoms[0].shells.size() - 1].basisFunctions.size() > firstAtomOrbsToMerge.size())
                                firstAtomOrbsToMerge.push_back(molecule.atoms[0].shells[molecule.atoms[0].shells.size() - 1].basisFunctions[firstAtomOrbsToMerge.size()]);
                        }
                    }
                    else
                    {
                        firstAtomOrbsToMerge.push_back(molecule.atoms[0].shells[curShell].basisFunctions[curOrb]);

                        if (molecule.atoms[1].shells[molecule.atoms[1].shells.size() - 1].basisFunctions.size() > curOrb)
                            secondAtomOrbsToMerge.push_back(molecule.atoms[1].shells[molecule.atoms[1].shells.size() - 1].basisFunctions[curOrb]);
                    }
                }
                else
                    orbitals.push_back(molecule.atoms[curAtom].shells[curShell].basisFunctions[curOrb]);


                currentParticles[curParticle] = molecule.atoms[curAtom].position + random.getRandomVector() * radiusInit;
                ++curParticle;
            }
        }
    }

    if (isSpecialMolecule)
    {
        unsigned int start = static_cast<unsigned int>(std::min(firstAtomOrbsToMerge.size(), secondAtomOrbsToMerge.size()));

        if (firstAtomOrbsToMerge.size() > start)
        {
            for (unsigned int i = 0; i < start; ++i)
                orbitals.push_back(firstAtomOrbsToMerge[i]);
        }

        if (secondAtomOrbsToMerge.size() > start)
        {
            for (unsigned int i = 0; i < start; ++i)
                orbitals.push_back(secondAtomOrbsToMerge[i]);
        }

        // merge orbitals
        for (unsigned int i = 0; i < start && orbitals.size() < curParticle; ++i)
        {
            const unsigned int index1 = firstAtomOrbsToMerge.size() == start ? i : start + i;
            const unsigned int index2 = secondAtomOrbsToMerge.size() == start ? i : start + i;

            const double Overlap = getOverlap(molecule.atoms[0], firstAtomOrbsToMerge[index1], molecule.atoms[1], secondAtomOrbsToMerge[index2]);

            orbitals.push_back(Orbitals::VQMCOrbital(firstAtomOrbsToMerge[index1], secondAtomOrbsToMerge[index2], Overlap, false));
        }

        for (unsigned int i = 0; i < start && orbitals.size() < curParticle; ++i)
        {
            const unsigned int index1 = firstAtomOrbsToMerge.size() == start ? i : start + i;
            const unsigned int index2 = secondAtomOrbsToMerge.size() == start ? i : start + i;

            const double Overlap = getOverlap(molecule.atoms[0], firstAtomOrbsToMerge[index1], molecule.atoms[1], secondAtomOrbsToMerge[index2]);

            orbitals.push_back(Orbitals::VQMCOrbital(firstAtomOrbsToMerge[index1], secondAtomOrbsToMerge[index2], Overlap, true));
        }
    }

    assert(curParticle == molecule.alphaElectrons);

    curParticle = 0;
    accumAtomParticles = 0;

    firstAtomOrbsToMerge.clear();
    secondAtomOrbsToMerge.clear();

    for (unsigned int curAtom = 0; curAtom < molecule.atoms.size(); ++curAtom)
    {
        accumAtomParticles += molecule.atoms[curAtom].betaElectrons;

        for (unsigned int curShell = 0; curShell < molecule.atoms[curAtom].shells.size(); ++curShell)
        {
            const bool isOnLastShell = curShell == molecule.atoms[curAtom].shells.size() - 1;

            for (unsigned int curOrb = 0; curOrb < molecule.atoms[curAtom].shells[curShell].basisFunctions.size(); ++curOrb)
            {
                if (curParticle >= accumAtomParticles)
                    break;

                if (isSpecialMolecule && isOnLastShell)
                {
                    if (curAtom) // on the second one
                    {
                        if (secondAtomOrbsToMerge.size() <= curOrb) // maybe already added when first atom was considered
                            secondAtomOrbsToMerge.push_back(molecule.atoms[1].shells[curShell].basisFunctions[curOrb]);

                        if (secondAtomOrbsToMerge.size() > firstAtomOrbsToMerge.size())
                        {
                            // try to add an uncompleted orbital from the first atom (last shell, of course), if possible
                            if (molecule.atoms[0].shells[molecule.atoms[0].shells.size() - 1].basisFunctions.size() > firstAtomOrbsToMerge.size())
                                firstAtomOrbsToMerge.push_back(molecule.atoms[0].shells[molecule.atoms[0].shells.size() - 1].basisFunctions[firstAtomOrbsToMerge.size()]);
                        }
                    }
                    else
                    {
                        firstAtomOrbsToMerge.push_back(molecule.atoms[0].shells[curShell].basisFunctions[curOrb]);

                        if (molecule.atoms[1].shells[molecule.atoms[1].shells.size() - 1].basisFunctions.size() > curOrb)
                            secondAtomOrbsToMerge.push_back(molecule.atoms[1].shells[molecule.atoms[1].shells.size() - 1].basisFunctions[curOrb]);
                    }
                }
                else
                    orbitals.push_back(molecule.atoms[curAtom].shells[curShell].basisFunctions[curOrb]);

                currentParticles[static_cast<unsigned long long int>(molecule.alphaElectrons) + curParticle] = molecule.atoms[curAtom].position + random.getRandomVector() * radiusInit;
                ++curParticle;
            }
        }
    }

    if (isSpecialMolecule)
    {
        unsigned int start = static_cast<unsigned int>(std::min(firstAtomOrbsToMerge.size(), secondAtomOrbsToMerge.size()));

        if (firstAtomOrbsToMerge.size() > start)
        {
            for (unsigned int i = 0; i < start; ++i)
                orbitals.push_back(firstAtomOrbsToMerge[i]);
        }

        if (secondAtomOrbsToMerge.size() > start)
        {
            for (unsigned int i = 0; i < start; ++i)
                orbitals.push_back(secondAtomOrbsToMerge[i]);
        }

        // merge orbitals
        for (unsigned int i = 0; i < start && orbitals.size() < static_cast<unsigned long long int>(molecule.alphaElectrons) + curParticle; ++i)
        {
            const unsigned int index1 = firstAtomOrbsToMerge.size() == start ? i : start + i;
            const unsigned int index2 = secondAtomOrbsToMerge.size() == start ? i : start + i;

            const double Overlap = getOverlap(molecule.atoms[0], firstAtomOrbsToMerge[index1], molecule.atoms[1], secondAtomOrbsToMerge[index2]);

            orbitals.push_back(Orbitals::VQMCOrbital(firstAtomOrbsToMerge[index1], secondAtomOrbsToMerge[index2], Overlap, false));
        }

        for (unsigned int i = 0; i < start && orbitals.size() < static_cast<unsigned long long int>(molecule.alphaElectrons) + curParticle; ++i)
        {
            const unsigned int index1 = firstAtomOrbsToMerge.size() == start ? i : start + i;
            const unsigned int index2 = secondAtomOrbsToMerge.size() == start ? i : start + i;

            const double Overlap = getOverlap(molecule.atoms[0], firstAtomOrbsToMerge[index1], molecule.atoms[1], secondAtomOrbsToMerge[index2]);

            orbitals.push_back(Orbitals::VQMCOrbital(firstAtomOrbsToMerge[index1], secondAtomOrbsToMerge[index2], Overlap, true));
        }
    }

    overlapIntegralsMap.clear();

    assert(curParticle == molecule.betaElectrons);

    SpinUpInvSlater.resize(molecule.alphaElectrons, molecule.alphaElectrons);
    SpinDownInvSlater.resize(molecule.betaElectrons, molecule.betaElectrons);

    //ComputeSlaterInv();
}


void Wavefunction::ComputeSlaterInv()
{
    for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i)
        for (unsigned int j = 0; j < m_molecule->alphaElectrons; ++j)
            SpinUpInvSlater(i, j) = orbitals[j](currentParticles[i]);

    if (m_molecule->alphaElectrons)
        SpinUpInvSlater = SpinUpInvSlater.inverse();

    for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
        for (unsigned int j = 0; j < m_molecule->betaElectrons; ++j)
            SpinDownInvSlater(i, j) = orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + j](currentParticles[static_cast<size_t>(m_molecule->alphaElectrons) + i]);

    if (m_molecule->betaElectrons)
        SpinDownInvSlater = SpinDownInvSlater.inverse();
}




double Wavefunction::SlaterRatio(const Vector3D<double>& newPos, unsigned int p) const
{
    double ratio = 0;

    // see 16.9
    if (p < m_molecule->alphaElectrons)
    {
        for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i) 
            ratio += orbitals[i](newPos) * SpinUpInvSlater(i, p);   
    }
    else
    {
        p -= m_molecule->alphaElectrons;

        for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
            ratio += orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + i](newPos) * SpinDownInvSlater(i, p);
    }

    return ratio;
}

double Wavefunction::JastrowRatio(const Vector3D<double>& newPos, unsigned int p, double beta) const
{
    double ratio = 0;

    // see 16.22
    
    const unsigned int Ne = m_molecule->alphaElectrons + m_molecule->betaElectrons;
    const bool MovedSpinUp = IsSpinUp(p);
    for (unsigned int i = 0; i < Ne; ++i)
    {
        if (p != i) // this could be split in two sums to avoid checking each time, but I won't bother - in this case the check also could be avoided because the difs below is zero for equality
        {
            const bool OtherSpinUp = IsSpinUp(i);

            const double difNew = (currentParticles[i] - newPos).Length();
            const double difOld = (currentParticles[i] - currentParticles[p]).Length();

            ratio += (difNew / (1. + beta * difNew) - difOld / (1. + beta * difOld)) * Alpha(MovedSpinUp, OtherSpinUp);
        }
    }

    return exp(ratio);
}


Vector3D<double> Wavefunction::HalfQuantumForce(const Vector3D<double>& newPos, unsigned int p, double beta, double ratio) const
{
    Vector3D<double> logGradientJastrow;

    // see 16.10, grad Psi / Psi can be decomposed in three sums, one of them is zero
    
    const bool MovedSpinUp = IsSpinUp(p);
    
    // Jastrow 
    // see 16.31 and 16.34
    const unsigned int Ne = m_molecule->alphaElectrons + m_molecule->betaElectrons;
    for (unsigned int i = 0; i < Ne; ++i)
    {
        if (p != i) // this could be split in two sums to avoid checking each time, but I won't bother
        {
            const bool OtherSpinUp = IsSpinUp(i);

            const Vector3D<double> difVec = newPos - currentParticles[i];
            const double difLength = difVec.Length();

            const double onepbetar = 1. + beta * difLength;

            const double fact = Alpha(MovedSpinUp, OtherSpinUp) / (onepbetar * onepbetar * difLength); // see 16.34

            logGradientJastrow += fact * difVec;
        }
    }

    Vector3D<double> logGradientSlater;

    // Slater
    // see 16.11 and 16.12
    if (p < m_molecule->alphaElectrons)
    {
        for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i)
            logGradientSlater += orbitals[i].getGradient(newPos) * SpinUpInvSlater(i, p);
    }
    else
    {
        p -= m_molecule->alphaElectrons;

        for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
            logGradientSlater += orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + i].getGradient(newPos) * SpinDownInvSlater(i, p);
    }

    return logGradientJastrow + logGradientSlater / ratio;
}


double Wavefunction::LocalKineticEnergy(double beta) const
{
    // the local kinetic energy is (-p^2 / 2 Psi_trial) / Psi_trial
    // expanding this you end up with 6 terms, 3 involving the laplacian for jastrow and each Slater determinant (divided by the corresponding value)
    // 3 involving products of gradients (divided by corresponding products of values)
    // one of each 3 terms is zero for a particular particle
    // see 16.16

    // the gradients computation is done as above in the HalfQuantumForce implementation
    // the new thing is the computation of the terms involving the laplacians

    const unsigned int Ne = m_molecule->alphaElectrons + m_molecule->betaElectrons;

    double laplacianJastrowRatio = 0;
    double laplacianSlaterRatio = 0;

    double gradientRatiosProd = 0;

    for (unsigned int p = 0; p < Ne; ++p)
    {
        const bool PartSpinUp = IsSpinUp(p);
        const Vector3D<double> partPos = currentParticles[p];

        // Jastrow 
        // see 16.31 and 16.34 for gradient
        // 16.38 and 16.39 for laplacian
        const unsigned int Ne = m_molecule->alphaElectrons + m_molecule->betaElectrons;
        Vector3D<double> logGradientJastrowPart;
        for (unsigned int i = 0; i < Ne; ++i)
        {
            if (p != i) // this could be split in two sums to avoid checking each time, but I won't bother
            {
                const bool OtherSpinUp = IsSpinUp(i);

                const Vector3D<double> difVec = partPos - currentParticles[i];
                const double difLength = difVec.Length();

                const double onepbetar = 1. + beta * difLength;
                const double onepbetar2 = onepbetar * onepbetar;
                const double alpha = Alpha(PartSpinUp, OtherSpinUp);

                const double fact = alpha / (onepbetar2 * difLength);

                logGradientJastrowPart += fact * difVec;

                laplacianJastrowRatio += 2. * (fact - alpha * beta / (onepbetar2 * onepbetar)); // see 16.34 and 16.39
            }
        }

        laplacianJastrowRatio += logGradientJastrowPart * logGradientJastrowPart; // see 16.38, first term

        // Slater
        // see 16.11 and 16.12 for gradient
        // 16.17 for laplacian

        Vector3D<double> logGradientSlaterPart;
        if (p < m_molecule->alphaElectrons)
        {
            for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i)
            {
                logGradientSlaterPart += orbitals[i].getGradient(partPos) * SpinUpInvSlater(i, p);
                laplacianSlaterRatio += orbitals[i].getLaplacian(partPos) * SpinUpInvSlater(i, p);
            }
        }
        else
        {
            const unsigned int lp = p - m_molecule->alphaElectrons;

            for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
            {
                logGradientSlaterPart += orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + i].getGradient(partPos)* SpinDownInvSlater(i, lp);
                laplacianSlaterRatio += orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + i].getLaplacian(partPos)* SpinDownInvSlater(i, lp);
            }
        }

        gradientRatiosProd += logGradientJastrowPart * logGradientSlaterPart;
    }

    return -0.5 * (laplacianJastrowRatio + laplacianSlaterRatio) - gradientRatiosProd;
}


double Wavefunction::LocalEnergy(double beta) const
{
    double potential = 0;

    // contribution from electron-nuclei potential
    for (const Vector3D<double>& v : currentParticles)
        for (const Systems::AtomWithShells& atom : m_molecule->atoms)
            potential -= static_cast<double>(atom.Z) / (v - atom.position).Length();

    // contribution from electron-electron potential
    for (unsigned int p1 = 1; p1 < currentParticles.size(); ++p1)
        for (unsigned int p2 = 0; p2 < p1; ++p2)
            potential += 1. / (currentParticles[p1] - currentParticles[p2]).Length();

    return LocalKineticEnergy(beta) + potential;
}


void Wavefunction::UpdateInverses(const Vector3D<double>& newPos, unsigned int p, double ratio)
{
    // see 16.18
    if (p < m_molecule->alphaElectrons)
    {
        // spin up
        Eigen::MatrixXd newInvSlater(m_molecule->alphaElectrons, m_molecule->alphaElectrons);

        for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i)
            newInvSlater(i, p) = SpinUpInvSlater(i, p) / ratio;

        for (unsigned int i = 0; i < m_molecule->alphaElectrons; ++i)
        {
            if (p != i)  // this could be split in two sums to avoid checking each time, but I won't bother
            {
                double sum = 0;
                for (unsigned int j = 0; j < m_molecule->alphaElectrons; ++j)
                    sum += orbitals[j](newPos) * SpinUpInvSlater(j, i);

                for (unsigned int j = 0; j < m_molecule->alphaElectrons; ++j)
                    newInvSlater(j, i) = SpinUpInvSlater(j, i) - SpinUpInvSlater(j, p) / ratio * sum;
            }
        }

        SpinUpInvSlater.swap(newInvSlater);
    }
    else
    {
        // spin down
        Eigen::MatrixXd newInvSlater(m_molecule->betaElectrons, m_molecule->betaElectrons);

        p -= m_molecule->alphaElectrons;

        for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
            newInvSlater(i, p) = SpinDownInvSlater(i, p) / ratio;

        for (unsigned int i = 0; i < m_molecule->betaElectrons; ++i)
        {
            if (p != i) // this could be split in two sums to avoid checking each time, but I won't bother
            {
                double sum = 0;
                for (unsigned int j = 0; j < m_molecule->betaElectrons; ++j)
                    sum += orbitals[static_cast<size_t>(m_molecule->alphaElectrons) + j](newPos) * SpinDownInvSlater(j, i);

                for (unsigned int j = 0; j < m_molecule->betaElectrons; ++j)
                    newInvSlater(j, i) = SpinDownInvSlater(j, i) - SpinDownInvSlater(j, p) / ratio * sum;
            }
        }

        
        SpinDownInvSlater.swap(newInvSlater);
    }
}


double Wavefunction::log_deriv_beta_wf(double beta) const
{
    const unsigned int numParticles = static_cast<unsigned int>(currentParticles.size());

    double val = 0;

    for (unsigned int i = 1; i < numParticles; ++i)
    {
        const bool Spin1Up = IsSpinUp(i);

        for (unsigned int j = 0; j < i; ++j)
        {
            const bool Spin2Up = IsSpinUp(j);

            const Vector3D<double> difRVec = currentParticles[i] - currentParticles[j];
            const double len2 = difRVec * difRVec;
            const double n = 1. + beta * sqrt(len2);

            val -= Wavefunction::Alpha(Spin1Up, Spin2Up) * len2 / (n * n);
        }
    }

    return val;
}


//************************************************************************************************************************************************************
// OVERLAP integrals
//************************************************************************************************************************************************************

double Wavefunction::getOverlap(const Systems::AtomWithShells& atom1, const Orbitals::GaussianOrbital& gaussian1, const Systems::AtomWithShells& atom2, const Orbitals::GaussianOrbital& gaussian2, bool extendForKinetic)
{
    std::tuple<unsigned int, unsigned int, double, double > params(gaussian1.shellID, gaussian2.shellID, gaussian1.alpha, gaussian2.alpha);

    auto it = overlapIntegralsMap.find(params);
    if (overlapIntegralsMap.end() != it) return it->second.getOverlap(gaussian1.angularMomentum, gaussian2.angularMomentum);


    // unfortunately it's not yet calculated
    GaussianIntegrals::GaussianOverlap overlap;
    auto result = overlapIntegralsMap.insert(std::make_pair(params, overlap));



    Orbitals::QuantumNumbers::QuantumNumbers maxQN1(0, 0, 0), maxQN2(0, 0, 0);

    // now find the max quantum numbers

    atom1.GetMaxQN(gaussian1.alpha, maxQN1);
    atom2.GetMaxQN(gaussian2.alpha, maxQN2);

    if (extendForKinetic)
    {
        // calculating the kinetic integrals needs +1 quantum numbers for overlap integrals
        ++maxQN1.l;
        ++maxQN1.m;
        ++maxQN1.n;

        ++maxQN2.n;
        ++maxQN2.l;
        ++maxQN2.m;
    }

    // calculate the integrals and that's about it

    result.first->second.Reset(gaussian1.alpha, gaussian2.alpha, gaussian1.center, gaussian2.center, maxQN1, maxQN2);

    return result.first->second.getOverlap(gaussian1.angularMomentum, gaussian2.angularMomentum);
}


double Wavefunction::getOverlap(const Systems::AtomWithShells& atom1, const Orbitals::ContractedGaussianOrbital& orbital1, const Systems::AtomWithShells& atom2, const Orbitals::ContractedGaussianOrbital& orbital2, bool extendForKinetic)
{
    double res = 0;

    for (auto& gaussian1 : orbital1.gaussianOrbitals)
        for (auto& gaussian2 : orbital2.gaussianOrbitals)
            res += gaussian1.normalizationFactor * gaussian2.normalizationFactor * gaussian1.coefficient * gaussian2.coefficient * getOverlap(atom1, gaussian1, atom2, gaussian2, extendForKinetic);

    return res;
}
