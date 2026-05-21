#include "PMActionInitialization.hh"
#include "PMPrimaryGenerator.hh"
#include "PMRunAction.hh"
#include "PMEventAction.hh"
#include "PMSteppingAction.hh"

PMActionInitialization::PMActionInitialization(G4double energy)
  : G4VUserActionInitialization(), fEnergy(energy) {}

PMActionInitialization::~PMActionInitialization() = default;

void PMActionInitialization::BuildForMaster() const {
  SetUserAction(new PMRunAction(fEnergy));
}

void PMActionInitialization::Build() const {
  fPrimaryGen = new PMPrimaryGenerator(fEnergy);     
  SetUserAction(fPrimaryGen);

  auto* eventAction = new PMEventAction();
  SetUserAction(eventAction);
  SetUserAction(new PMSteppingAction(eventAction));
  SetUserAction(new PMRunAction(fEnergy));
}
