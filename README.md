# Machine Learning-Based Pulse Shape Discrimination in Plastic Scintillators using Geant4 Simulations

This repository contains the full Geant4 simulation framework developed as the core computational engine for my **Bachelor of Science Thesis in Physics Engineering** at **Istanbul Technical University**. 

---

## 🎯 Thesis Overview & The Role of Geant4

The primary objective of this thesis is to achieve robust **Neutron-Gamma Pulse Shape Discrimination (PSD)** in organic plastic scintillators. Traditional methods compress high-dimensional waveforms into a single scalar parameter (such as the classical Charge Comparison method), making them highly susceptible to performance degradation at lower energies due to photon-counting noise and temporal dispersion.

To overcome this limitation, this project develops a **Time-Domain, Waveform-Level Particle Identification** strategy using machine learning.

### The Critical Role of Geant4 in the Thesis:
* **Microscopic Modeling:** Instead of relying on analytical approximations, Geant4 explicitly models the birth, transport, scattering, and detection of individual optical photons event-by-event.
* **Realistic Signal Convolution:** The simulated waveforms naturally convolve the intrinsic scintillation kinetics with optical transport physics, including surface reflections, path-length variations, and boundary absorption.
* **Synthetic Dataset Generation:** Geant4 operates as a physics-consistent data generator. By conducting precise energy scans across the MeV regime (1.0 to 4.0 MeV), it creates the clean, labeled high-dimensional datasets ($x=[n_1, n_2, ..., n_{5000}]$) required to train a Multilayer Perceptron (MLP) classifier, which ultimately achieved a **93% classification accuracy**.

---

## 🔬 Core Physics Configurations & Critical Code Locations

To ensure reproducibility, the detailed implementation of material properties, optical photon physics, and geometry boundaries are strategically modularized.

### 1. Material Definitions & Optical Properties
> **📂 Critical File Location:** `src/PMDetectorConstruction.cc`

All physical media, along with their wavelength-dependent optical parameters, are defined via `G4MaterialPropertiesTable` (MPT) objects inside this file:

* **Air (`CreateAirMaterial()`):** Defined with a constant refractive index ($n=1.0$) across the optical photon energy spectrum to fill the experimental air gaps and holes.
* **Teflon Wrapping (`CreateTeflonMaterial()`):** Implemented using the standard NIST material database (`G4_TEFLON`) and assigned a refractive index of $n=1.35$ to act as a realistic cladding layer.
* **EJ-276 Plastic Scintillator (`CreateScintillatorMaterial()`):**
  * Re-implemented using an advanced **3-component scintillation decay model** (based on Holroyd 2025 constants) to enable realistic pulse shape discrimination.
  * Explicitly populates fast ($\tau_1 = 13.0\text{ ns}$), medium ($\tau_2 = 35.0\text{ ns}$), and slow ($\tau_3 = 270.0\text{ ns}$) decay channels.
  * Encodes unique relative scintillation weights to represent the prompt and delayed light emission characteristics ($w_1 = 0.76$, $w_2 = 0.18$, $w_3 = 0.06$).
  * Activates **Birks Quenching** (`SetBirksConstant(0.0125 * cm/MeV)`) to correctly account for the light output reduction caused by the high ionization density of recoil protons.

### 2. Optical Photon & Scintillation Process Tracking
> **📂 Critical File Locations:** `src/PMPhysicsList.cc` & `src/PMSteppingAction.cc`

Optical photon tracking and process handling follow precise configuration protocols:

* **Physics List Framework (`PMPhysicsList.cc`):**
  * Built on a modular foundation combining **`QGSP_BIC_HP`** (for high-precision data-driven hadronic/neutron transport) and **`G4EmStandardPhysics_option4`** (for low-energy electromagnetic accuracy).
  * **Critical Parameter Selection:** `opticalParams->SetScintByParticleType(false)` is strictly enforced. Instead, particle-dependent scintillation decay constants are driven through specific initial state populating weights via the custom `ensureScint` wrapper.
  * **Birks Quenching Integration:** Attached directly to the standard EM saturation manager (`G4LossTableManager::Instance()->EmSaturation()`) and mapped onto $e^-$, $e^+$, proton, and $\alpha$ tracks to handle physical quenching seamlessly.
* **Step-by-Step Signal Construction (`PMSteppingAction.cc`):**
  * Monitors tracks at the microscopic level. If the particle definition matches `G4OpticalPhoton::OpticalPhotonDefinition()`, it isolates the photon step immediately.
  * Captures the exact **Global Time** of photon arrivals at the active photodetector boundary (`physAlDetector`). These time stamps are pushed into `PMEventAction` to build the final time-domain histograms.

### 3. Boundary & Photodetector Definition
> **📂 Critical File Location:** `src/PMDetectorConstruction.cc` (Inside `Construct()`)

The detector is modeled as an isotropic cubic volume with fixed dimensions of $10\times10\times10\text{ cm}^3$. Five of its faces are wrapped in a thin ($10\ \mu\text{m}$) highly reflective Teflon layer modeled via a unified `polished` dielectric-metal optical interface with $99\%$ reflectivity. 

The negative X-face strips away this reflective cladding and features a dedicated 1 cm diameter aperture opening into an independent, non-reflective Aluminum plate that serves as the effective photodetector surface. The boundary is assigned a custom `AluminumDetectorSurface` optical table where `REFLECTIVITY = 0.0` and `EFFICIENCY = 1.0`, ensuring that every arriving optical photon passing through the hole is absorbed and recorded as a digitized signal hit.


1. **Primary Action:** Generates isotropic center-sourced particles. Dynamically passes the primary PDG code (Gamma: 22, Neutron: 2112) to the Event Action at the vertex level.
2. **Event & Sensitive Tracking:** `PMEventAction` manages the underlying memory buffers, creating time-binned arrays (`fArrivalHist` and `fEmissionHist`). It separates pulses into unique analytical rows (`optical_arrival`, `optical_emission`, `edep_time`).
3. **Run Termination:** `PMRunAction` handles thread-safe merging (`SetNtupleMerging(true)`) and dumps high-resolution wave columns directly into dedicated structural `.root` and `.csv` files for downstream Machine Learning models.

---

## 🚀 Getting Started & Build Instructions

### Prerequisites
* Geant4 (Recommended: 11.0 or higher) compiled with **QT Visualization** and **Multithreading** enabled.
* CMake (v3.16+)
* A standard C++17 compatible compiler (`gcc`, `clang`)

### Building the Project
```bash
# Clone the repository
git clone [https://github.com/TubaSatoglu/PSD-in-Plastic-Scintillator-in-Geant4-using-MLP.git](https://github.com/TubaSatoglu/PSD-in-Plastic-Scintillator-in-Geant4-using-MLP.git)
cd PSD-in-Plastic-Scintillator-in-Geant4-using-MLP

# Create a build directory
mkdir build && cd build

# Configure and compile
cmake ..
make -j$(nproc)


### Running the Simulation
To execute the simulation in interactive GUI mode:
Bash
./sim


To run in batch mode using a macro file (e.g., for automated multi-energy data collection scans):
Bash
./sim run.mac
