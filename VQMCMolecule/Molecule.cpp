#include "Molecule.h"

#include <algorithm>
#include <numeric>
#include <functional>

namespace Systems {


	Molecule::Molecule()
		: alphaElectrons(0), betaElectrons(0)
	{
	}


	unsigned int Molecule::CountNumberOfContractedGaussians() const
	{
		unsigned int res = 0;

		for (const auto& atom : atoms)
			res += atom.CountNumberOfContractedGaussians();

		return res;
	}

	unsigned int Molecule::CountNumberOfGaussians() const
	{
		unsigned int res = 0;

		for (const auto& atom : atoms)
			res += atom.CountNumberOfGaussians();

		return res;
	}

}

void Systems::Molecule::SetIDs()
{
	unsigned int ID = 0;
	unsigned int contractedID = 0;
	unsigned int shellID = 0;
	unsigned int atomID = 0;

	for (auto& atom : atoms)
	{
		atom.ID = atomID++;
		
		for (auto& shell : atom.shells)
		{
			shell.ID = shellID++;
			shell.centerID = atom.ID;

			for (auto& orbital : shell.basisFunctions)
			{
				orbital.ID = contractedID++;
				orbital.centerID = atom.ID;
				orbital.shellID = shell.ID;

				for (auto& gaussian : orbital.gaussianOrbitals)
				{
					gaussian.ID = ID++;
					gaussian.centerID = atom.ID;
					gaussian.shellID = shell.ID;
				}
			}
		}
	}
}


double Systems::Molecule::NuclearRepulsionEnergy() const
{
	double energy = 0;

	for (unsigned int atom1 = 0; atom1 < atoms.size(); ++atom1)
		for (unsigned int atom2 = atom1 + 1; atom2 < atoms.size(); ++atom2)
			energy += static_cast<double>(atoms[atom1].Z) * atoms[atom2].Z / (atoms[atom1].position - atoms[atom2].position).Length();

	return energy;
}


unsigned int Systems::Molecule::ElectronsNumber()
{
	if (alphaElectrons > 0 || betaElectrons > 0) return alphaElectrons + betaElectrons;

	return static_cast<unsigned int>(std::accumulate(atoms.begin(), atoms.end(), 0, [](unsigned int result, const AtomWithShells& atom) { return result + atom.electronsNumber; }));
}


unsigned int Systems::Molecule::GetMaxAngularMomentum()
{
	unsigned int L = 0;

	for (const auto &atom : atoms)
			L = std::max(L, atom.GetMaxAngularMomentum());

	return L;
}


void Systems::Molecule::SetCenterForShells()
{
	for (auto &atom : atoms) atom.SetCenterForShells();
}


void Systems::Molecule::Normalize()
{
	for (auto &atom : atoms) atom.Normalize();
}


void Systems::Molecule::Init()
{
	SetCenterForShells();
	SetIDs();

	for (auto& atom : atoms)
	{
		atom.alphaElectrons = 0;
		int totalElectrons = atom.Z;
		for (int n = 1; ; ++n) // for each shell
		{
			const int n2 = n * n;
			const int maxElectronsInShell = 2 * n2;
			if (totalElectrons < maxElectronsInShell)
			{
				for (int l = 0; l < n; ++l) // for each subshell
				{
					const int maxAlphaElectronsInSubshell = 2 * l + 1;
					const int maxElectronsInSubshell = 2 * maxAlphaElectronsInSubshell;

					if (totalElectrons < maxElectronsInSubshell)
					{
						if (totalElectrons > maxAlphaElectronsInSubshell)
							atom.alphaElectrons += maxAlphaElectronsInSubshell;
						else
							atom.alphaElectrons += totalElectrons;

						break;
					}

					totalElectrons -= maxElectronsInSubshell;
					atom.alphaElectrons += maxAlphaElectronsInSubshell;
				}
				
				break;
			}

			atom.alphaElectrons += n2;
			totalElectrons -= maxElectronsInShell;
		}

		atom.betaElectrons = atom.electronsNumber - atom.alphaElectrons;
	}

	for (int i = 0; i < atoms.size(); ++i)
		if (i % 2) std::swap(atoms[i].alphaElectrons, atoms[i].betaElectrons);

	alphaElectrons = static_cast<unsigned int>(std::accumulate(atoms.begin(), atoms.end(), 0, [](unsigned int result, const AtomWithShells& atom) { return result + atom.alphaElectrons; }));
	betaElectrons = static_cast<unsigned int>(std::accumulate(atoms.begin(), atoms.end(), 0, [](unsigned int result, const AtomWithShells& atom) { return result + atom.betaElectrons; }));
}
