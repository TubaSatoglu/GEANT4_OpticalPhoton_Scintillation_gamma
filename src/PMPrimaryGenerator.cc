#include "PMPrimaryGenerator.hh"
#include "PMEventAction.hh"

#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <cmath>

PMPrimaryGenerator::PMPrimaryGenerator(G4double energyMeV)
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(new G4ParticleGun(1)),
  fBaseEnergy(energyMeV * MeV),
  fEnergySigma(0.0),          // monoenergetic
  fSourcePosZ(0.0 * cm),      // center source
  fSourceRadius(0.0 * cm),
  fBeamTheta(0.0 * deg)       // unused for isotropic mode
{
    auto* gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(gamma);

    G4cout << "[PMPrimaryGenerator] Center isotropic source initialized.\n"
           << "  Energy = " << energyMeV << " MeV\n"
           << "  Default particle = gamma\n";
}

PMPrimaryGenerator::~PMPrimaryGenerator()
{
    delete fParticleGun;
}

void PMPrimaryGenerator::GeneratePrimaries(G4Event* event)
{
    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 10.5 * cm));

    // Isotropic emission: sample full solid angle
    const G4double cosT = 2.0 * G4UniformRand() - 1.0;
    const G4double sinT = std::sqrt(1.0 - cosT*cosT);
    const G4double phi  = 2.0 * M_PI * G4UniformRand();

    const G4double ux = sinT * std::cos(phi);
    const G4double uy = sinT * std::sin(phi);
    const G4double uz = cosT;

    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(ux, uy, uz));

    // Set monoenergetic energy
    fParticleGun->SetParticleEnergy(fBaseEnergy);

    // Inform EventAction about PDG of the primary
    const G4UserEventAction* baseEA =
        G4RunManager::GetRunManager()->GetUserEventAction();

    if (baseEA)
    {
        // Safe const-correct cast
        auto* evtAction = dynamic_cast<const PMEventAction*>(baseEA);

        if (evtAction)
        {
            // Remove const only for PDG assignment (safe)
            const_cast<PMEventAction*>(evtAction)
                ->SetPrimaryPDG(fParticleGun->GetParticleDefinition()->GetPDGEncoding());
        }
    }

    // Create the primary vertex
    fParticleGun->GeneratePrimaryVertex(event);
}
