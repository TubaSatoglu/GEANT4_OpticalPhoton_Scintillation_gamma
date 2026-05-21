#include "PMPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4OpticalPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4OpticalParameters.hh"

#include "G4OpticalPhoton.hh"
#include "G4ProcessManager.hh"
#include "G4Scintillation.hh"
#include "G4Cerenkov.hh"

#include "G4Gamma.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
#include "G4Proton.hh"
#include "G4Alpha.hh"

#include "G4BosonConstructor.hh"
#include "G4LeptonConstructor.hh"
#include "G4MesonConstructor.hh"
#include "G4BaryonConstructor.hh"
#include "G4IonConstructor.hh"
#include "G4ShortLivedConstructor.hh"

#include "G4LossTableManager.hh"
#include "G4EmSaturation.hh"
#include "G4SystemOfUnits.hh"

PMPhysicsList::PMPhysicsList()
  : G4VModularPhysicsList()
{
  SetVerboseLevel(1);

  // EMZ (option4)
  RegisterPhysics(new G4EmStandardPhysics_option4());

  // Hadronic + decay
  RegisterPhysics(new G4DecayPhysics());
  RegisterPhysics(new G4RadioactiveDecayPhysics());
  RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  RegisterPhysics(new G4IonPhysics());
  RegisterPhysics(new G4StoppingPhysics());

  // Optical physics
  auto* opticalPhysics = new G4OpticalPhysics();
  RegisterPhysics(opticalPhysics);

  // Global optical parameters
  auto* opticalParams = G4OpticalParameters::Instance();
  opticalParams->SetScintByParticleType(false);          // IMPORTANT: must be false
  opticalParams->SetScintStackPhotons(true);
  opticalParams->SetScintTrackInfo(true);
  opticalParams->SetScintTrackSecondariesFirst(true);

  opticalParams->SetCerenkovStackPhotons(true);
  opticalParams->SetCerenkovMaxPhotonsPerStep(300);
  opticalParams->SetCerenkovMaxBetaChange(10.0);
  opticalParams->SetCerenkovTrackSecondariesFirst(true);

  G4cout << "Physics List: QGSP_BIC_HP + EMZ + Optical loaded." << G4endl;
}

PMPhysicsList::~PMPhysicsList() = default;

void PMPhysicsList::ConstructParticle()
{
  G4BosonConstructor().ConstructParticle();
  G4LeptonConstructor().ConstructParticle();
  G4MesonConstructor().ConstructParticle();
  G4BaryonConstructor().ConstructParticle();
  G4IonConstructor().ConstructParticle();
  G4ShortLivedConstructor().ConstructParticle();

  G4OpticalPhoton::OpticalPhotonDefinition();
}

void PMPhysicsList::ConstructProcess()
{
  G4VModularPhysicsList::ConstructProcess();

  auto* params = G4OpticalParameters::Instance();
  params->SetScintByParticleType(false);                // keep it false here as well
  params->SetScintStackPhotons(true);
  params->SetScintTrackInfo(true);
  params->SetScintTrackSecondariesFirst(true);

  params->SetCerenkovStackPhotons(true);
  params->SetCerenkovMaxPhotonsPerStep(300);
  params->SetCerenkovMaxBetaChange(10.0);
  params->SetCerenkovTrackSecondariesFirst(true);

  // Birks quenching via EmSaturation
  auto* sat = G4LossTableManager::Instance()->EmSaturation();

  auto ensureScint = [&](G4ParticleDefinition* p)
  {
    if (!p) return;
    auto* pm = p->GetProcessManager();
    if (!pm) return;

    G4Scintillation* sc = nullptr;
    for (int i = 0; i < pm->GetProcessListLength(); ++i) {
      sc = dynamic_cast<G4Scintillation*>((*pm->GetProcessList())[i]);
      if (sc) break;
    }

    if (!sc) {
      sc = new G4Scintillation("Scintillation");
      sc->SetTrackSecondariesFirst(true);
      sc->SetScintillationByParticleType(false);        // IMPORTANT: false
      pm->AddProcess(sc, 0, -1, 1);
      G4cout << "[Scint] attached to " << p->GetParticleName() << G4endl;
    } else {
      sc->SetScintillationByParticleType(false);        // force false even if default
    }

    sc->AddSaturation(sat);
  };

  ensureScint(G4Electron::ElectronDefinition());
  ensureScint(G4Positron::PositronDefinition());
  ensureScint(G4Proton::ProtonDefinition());
  ensureScint(G4Alpha::AlphaDefinition());

  G4cout << "\n=== Physics processes constructed ===" << G4endl;
  G4cout << "  Scintillation attached to e+/e-/p/alpha" << G4endl;
  G4cout << "  EmSaturation (Birks quenching) enabled"   << G4endl;
  G4cout << "  ScintByParticleType = FALSE"              << G4endl;
  G4cout << "======================================"   << G4endl;
}

void PMPhysicsList::SetCuts()
{
  SetCutsWithDefault();

  SetCutValue(0.01 * mm, "gamma");
  SetCutValue(0.01 * mm, "e-");
  SetCutValue(0.01 * mm, "e+");
  SetCutValue(0.01 * mm, "proton");
  SetCutValue(0.01 * mm, "alpha");

  G4cout << "\n=== Production cuts ===" << G4endl;
  G4cout << "  gamma  : 0.01 mm" << G4endl;
  G4cout << "  e-/e+  : 0.01 mm" << G4endl;
  G4cout << "  proton : 0.01 mm" << G4endl;
  G4cout << "  alpha  : 0.01 mm" << G4endl;
  G4cout << "========================\n" << G4endl;
}
