#ifndef NCrystal_Info_hh
#define NCrystal_Info_hh

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  This file is part of NCrystal (see https://mctools.github.io/ncrystal/)   //
//                                                                            //
//  Copyright 2015-2021 NCrystal developers                                   //
//                                                                            //
//  Licensed under the Apache License, Version 2.0 (the "License");           //
//  you may not use this file except in compliance with the License.          //
//  You may obtain a copy of the License at                                   //
//                                                                            //
//      http://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
//  Unless required by applicable law or agreed to in writing, software       //
//  distributed under the License is distributed on an "AS IS" BASIS,         //
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
//  See the License for the specific language governing permissions and       //
//  limitations under the License.                                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "NCrystal/NCSABData.hh"
#include "NCrystal/NCAtomData.hh"

/////////////////////////////////////////////////////////////////////////////////
// Data class containing information (high level or derived) about a given     //
// material. Instances of the class are typically generated by dedicated       //
// factories, based on interpretation of data files with e.g. crystallographic //
// information. Physics models in the form of for example NCScatter or         //
// NCAbsorption instances, are then initialised from these Info objects, thus  //
// providing a separation layer between data sources and algorithms working on //
// said data.                                                                  //
/////////////////////////////////////////////////////////////////////////////////

namespace NCrystal {

  class Info;
  class DynamicInfo;

  struct NCRYSTAL_API StructureInfo final {
    unsigned spacegroup = 0;//From 1-230 if provided, 0 if not available
    double lattice_a = 0.0;//angstrom
    double lattice_b = 0.0;//angstrom
    double lattice_c = 0.0;//angstrom
    double alpha = 0.0;//degree
    double beta = 0.0;//degree
    double gamma = 0.0;//degree
    double volume = 0.0;//Aa^3
    unsigned n_atoms = 0;//Number of atoms per unit cell
  };

  struct NCRYSTAL_API HKLInfo final : private MoveOnly {
    double dspacing = 0.0;//angstrom
    double fsquared = 0.0;//barn
    int h = 0;
    int k = 0;
    int l = 0;
    unsigned multiplicity = 0;

    //If the HKLInfo source knows the plane normals, they will be provided in
    //the following list as unit vectors. Only half of the normals should be
    //included in this list, since if n is a normal, so is -n. If demi_normals
    //is not empty, it will be true that multiplicity == 2*demi_normals.size().

    class NCRYSTAL_API Normal final : public StronglyTypedFixedVector<Normal,double,3> {
    public:
      using StronglyTypedFixedVector::StronglyTypedFixedVector;
    };

    std::vector<Normal> demi_normals;//TODO: vector->pointer saves 16B when not used

    //If eqv_hkl is not a null pointer, it contains the corresponding Miller
    //indices of the demi_normals as three 2-byte integers (short). Thus,
    //eqv_hkl has demi_normal.size()*3 entries:
    std::unique_ptr<short[]> eqv_hkl;
  };

  typedef std::vector<HKLInfo> HKLList;

  class NCRYSTAL_API AtomIndex final : public EncapsulatedValue<AtomIndex,unsigned> {
  public:
    using EncapsulatedValue::EncapsulatedValue;
    static constexpr const char * unit() noexcept { return ""; }
  };

  struct NCRYSTAL_API IndexedAtomData {
    //AtomData and associated index. The index is *only* valid in association
    //with a particular Info object. It exists since it is in principle possible
    //to have the same fundamental atom playing more than one role in a given
    //material (for instance, the same atom could have different displacements
    //on different positions in the unit cell).
    AtomDataSP atomDataSP;
    AtomIndex index;

    const AtomData& data() const ncnoexceptndebug;

    //Sort by index (comparison should only be performed with objects associated
    //with the same Info object):
    bool operator<(const IndexedAtomData& o) const ncnoexceptndebug;
    bool operator==(const IndexedAtomData& o) const ncnoexceptndebug;
  };

  class NCRYSTAL_API AtomInfo final : private MoveOnly {
  public:

    //////////////////////////////////////////////////////////////////////////////
    // Information about one kind of atom in a crystal unit cell, sharing both  //
    // atomic composition and dynamic behaviour (reflected in e.g. mean squared //
    // displacement values and associated DynamicInfo object).                  //
    //////////////////////////////////////////////////////////////////////////////

    //Always contains info about atomic compositions:
    ncconstexpr17 const IndexedAtomData& indexedAtomData() const noexcept { return m_iad; }
    ncconstexpr17 const IndexedAtomData& atom() const noexcept { return m_iad; }
    AtomDataSP atomDataSP() const noexcept { return m_iad.atomDataSP; }
    const AtomData& atomData() const ncnoexceptndebug { return m_iad.data(); }

    //Always contains non-empty list of associated unit cell positions:
    class NCRYSTAL_API Pos final : public StronglyTypedFixedVector<Pos,double,3> {
    public:
      using StronglyTypedFixedVector::StronglyTypedFixedVector;
    };
    using AtomPositions = std::vector<Pos>;
    const AtomPositions& unitCellPositions() const noexcept { return m_pos; }
    unsigned numberPerUnitCell() const noexcept { return static_cast<unsigned>( m_pos.size() ); }

    //Mean-square-displacements in angstrom^2 is optional. Note that this is the
    //displacement projected onto a linear axis, for direct usage in isotropic
    //Debye-Waller factors:
    ncconstexpr17 const Optional<double>& msd() const noexcept { return m_msd; }

    //Debye temperature is optional::
    ncconstexpr17 const Optional<DebyeTemperature>& debyeTemp() const noexcept { return m_dt; }

    //Corresponding DynamicInfo object on the same Info (returns nullptr if not available):
    const DynamicInfo* correspondingDynamicInfo() const { return m_dyninfo; }

    //Constructor:
    AtomInfo( IndexedAtomData, AtomPositions&&, Optional<DebyeTemperature>, Optional<double> msd );
  private:
    IndexedAtomData m_iad;
    Optional<DebyeTemperature> m_dt;
    Optional<double> m_msd;
    std::vector<Pos> m_pos;
    const DynamicInfo* m_dyninfo = nullptr;
    friend class Info;
  };

  using AtomInfoList = std::vector<AtomInfo>;
  using AtomList = AtomInfoList;//obsolete alias

  class NCRYSTAL_API DynamicInfo : public UniqueID {
  public:
    DynamicInfo(double fraction, IndexedAtomData, Temperature);
    virtual ~DynamicInfo() = 0;//Make abstract
    double fraction() const;
    void changeFraction(double f) { m_fraction = f; }
    Temperature temperature() const;//same as on associated Info object

    const IndexedAtomData& atom() const { return m_atom; }
    AtomDataSP atomDataSP() const { return m_atom.atomDataSP; }
    const AtomData& atomData() const { return m_atom.data(); }

    //Corresponding AtomInfo object on the same Info (nullptr if not available):
    const AtomInfo* correspondingAtomInfo() const { return m_atomInfo; }
  private:
    double m_fraction;
    IndexedAtomData m_atom;
    Temperature m_temperature;
    AtomInfo* m_atomInfo = nullptr;
    friend class Info;
  };

  typedef std::vector<std::unique_ptr<DynamicInfo>> DynamicInfoList;

  class NCRYSTAL_API DI_Sterile final : public DynamicInfo {
    //Class indicating elements for which inelastic neutron scattering is absent
    //or disabled.
  public:
    using DynamicInfo::DynamicInfo;
    virtual ~DI_Sterile();
  };

  class NCRYSTAL_API DI_FreeGas final : public DynamicInfo {
    //Class indicating elements for which inelastic neutron scattering should be
    //modelled as scattering on a free gas.
  public:
    using DynamicInfo::DynamicInfo;
    virtual ~DI_FreeGas();
  };

  class NCRYSTAL_API DI_ScatKnl : public DynamicInfo {
  public:
    //Base class for dynamic information which can, directly or indirectly,
    //result in a SABData scattering kernel. The class is mostly semantic, as no
    //SABData access interface is provided on this class, as some derived
    //classes (e.g. VDOS) need dedicated algorithms in order to create the
    //SABData object. This class does, however, provide a unified interface for
    //associated data which is needed in order to use the SABData for
    //scattering. Currently this is just the grid of energy points for which SAB
    //integration will perform analysis and caching.
    virtual ~DI_ScatKnl();

    //If source dictated what energy grid to use for caching cross-sections,
    //etc., it can be returned here. It is ok to return a null ptr, leaving the
    //decision entirely to the consuming code. Grids must have at least 3
    //entries, and grids of size 3 actually indicates [emin,emax,npts], where
    //any value can be 0 to leave the choice for the consuming code. Grids of
    //size >=4 most be proper grids.
    typedef std::shared_ptr<const VectD> EGridShPtr;
    virtual EGridShPtr energyGrid() const = 0;
  protected:
    using DynamicInfo::DynamicInfo;
  };

  class NCRYSTAL_API DI_ScatKnlDirect : public DI_ScatKnl {
  public:
    //Pre-calculated scattering kernel which at most needs a conversion to
    //SABData format before it is available. For reasons of efficiency, this
    //conversion is actually not required to be carried out before calling code
    //calls the MT-safe ensureBuildThenReturnSAB().
    using DI_ScatKnl::DI_ScatKnl;
    virtual ~DI_ScatKnlDirect();

    //Use ensureBuildThenReturnSAB to access the scattering kernel:
    std::shared_ptr<const SABData> ensureBuildThenReturnSAB() const;

    //check if SAB is already built:
    bool hasBuiltSAB() const;

  protected:
    //Implement in derived classes to build the completed SABData object (will
    //only be called once and in an MT-safe context, protected by per-object
    //mutex):
    virtual std::shared_ptr<const SABData> buildSAB() const = 0;
  private:
    mutable std::shared_ptr<const SABData> m_sabdata;
    mutable std::mutex m_mutex;
  };

  class NCRYSTAL_API DI_VDOS : public DI_ScatKnl {
  public:
    //For a solid material, a phonon spectrum in the form of a Vibrational
    //Density Of State (VDOS) parameterisation, can be expanded into a full
    //scattering kernel. The calling code is responsible for doing this,
    //including performing choices as to grid layout, expansion order, etc.
    using DI_ScatKnl::DI_ScatKnl;
    virtual ~DI_VDOS();
    virtual const VDOSData& vdosData() const = 0;

    //The above vdosData() function returns regularised VDOS. The following
    //functions provide optional access to the original curves (returns empty
    //vectors if not available):
    virtual const VectD& vdosOrigEgrid() const = 0;
    virtual const VectD& vdosOrigDensity() const = 0;
  };

  class NCRYSTAL_API DI_VDOSDebye final : public DI_ScatKnl {
  public:
    //An idealised VDOS spectrum, based on the Debye Model in which the spectrum
    //rises quadratically with phonon energy below a cutoff value, kT, where T
    //is the Debye temperature (which must be available on the associated Info
    //object).
    DI_VDOSDebye( double fraction,
                  IndexedAtomData,
                  Temperature temperature,
                  DebyeTemperature debyeTemperature );
    virtual ~DI_VDOSDebye();
    DebyeTemperature debyeTemperature() const;
    EGridShPtr energyGrid() const override { return nullptr; }
  private:
    DebyeTemperature m_dt;
  };

  class NCRYSTAL_API Info final : private MoveOnly {
  public:

    UniqueIDValue getUniqueID() const { return m_uid.getUniqueID(); }

    //////////////////////////
    // Check if crystalline //
    //////////////////////////

    //Materials can be crystalline (i.e. at least one of structure info, atomic
    //positions and hkl info) must be present. Non-crystalline materials must
    //always have dynamic info present.
    bool isCrystalline() const;

    /////////////////////////////////////////
    // Information about crystal structure //
    /////////////////////////////////////////

    bool hasStructureInfo() const;
    const StructureInfo& getStructureInfo() const;

    //Convenience method, calculating the d-spacing of a given Miller
    //index. Calling this incurs the overhead of creating a reciprocal lattice
    //matrix from the structure info:
    double dspacingFromHKL( int h, int k, int l ) const;

    /////////////////////////////////////////
    // Information about material dynamics //
    /////////////////////////////////////////

    bool hasDynamicInfo() const;
    const DynamicInfoList& getDynamicInfoList() const;

    /////////////////////////////////////////////
    // Information about cross-sections [barn] //
    /////////////////////////////////////////////

    //absorption cross-section (at 2200m/s):
    bool hasXSectAbsorption() const;
    SigmaAbsorption getXSectAbsorption() const;

    //saturated scattering cross-section (high E limit):
    bool hasXSectFree() const;
    SigmaFree getXSectFree() const;

    /////////////////////////////////////////////////////////////////////////////////
    // Provides calculation of "background" (non-Bragg diffraction) cross-sections //
    /////////////////////////////////////////////////////////////////////////////////

    bool providesNonBraggXSects() const;
    CrossSect xsectScatNonBragg(NeutronEnergy) const;

    ///////////////////////////
    // Temperature [kelvin]  //
    ///////////////////////////

    bool hasTemperature() const;
    Temperature getTemperature() const;

    ///////////////////////////////////
    // Atom Information in unit cell //
    ///////////////////////////////////

    bool hasAtomInfo() const;

    //NB: it is ok to access atom info list even if hasAtomInfo()==false (the
    //list will simply be empty and hasXXX will return false):
    AtomInfoList::const_iterator atomInfoBegin() const;
    AtomInfoList::const_iterator atomInfoEnd() const;
    const AtomInfoList& getAtomInfos() const;

    //Whether AtomInfo objects have mean-square-displacements available (they
    //will either all have them available, or none of them will have them
    //available):
    bool hasAtomMSD() const;

    //Whether AtomInfo objects have Debye temperatures available (they will
    //either all have them available, or none of them will have them available):
    bool hasAtomDebyeTemp() const;

    //Alias:
    bool hasDebyeTemperature() const { return hasAtomDebyeTemp(); }


    /////////////////////
    // HKL Information //
    /////////////////////

    //NB: it is ok to access hklList() (including nHKL, hklBegin, hklLast and
    //hklEnd even if hasHKLInfo()==false (the results will simply indicate empty
    //list):

    bool hasHKLInfo() const;
    unsigned nHKL() const;
    HKLList::const_iterator hklBegin() const;//first (==end if empty)
    HKLList::const_iterator hklLast() const;//last (==end if empty)
    HKLList::const_iterator hklEnd() const;
    const HKLList& hklList() const;
    //The limits:
    double hklDLower() const;
    double hklDUpper() const;
    //The largest/smallest (both returns inf if nHKL=0):
    double hklDMinVal() const;
    double hklDMaxVal() const;

    //////////////////////////////
    // Expanded HKL Information //
    //////////////////////////////

    //Whether HKLInfo objects have demi_normals available:
    bool hasHKLDemiNormals() const;

    //Whether HKLInfo objects have eqv_hkl available:
    bool hasExpandedHKLInfo() const;

    //Search eqv_hkl lists for specific (h,k,l) value. Returns hklEnd() if not found:
    HKLList::const_iterator searchExpandedHKL(short h, short k, short l) const;

    /////////////////////
    // Density [g/cm^3] //
    /////////////////////

    bool hasDensity() const;
    Density getDensity() const;

    /////////////////////////////////
    // Number density [atoms/Aa^3] //
    /////////////////////////////////

    bool hasNumberDensity() const;
    NumberDensity getNumberDensity() const;

    ////////////////////////////////////////////////////////////////////////////
    // Basic composition (always consistent with AtomInfo/DynInfo if present) //
    ////////////////////////////////////////////////////////////////////////////

    bool hasComposition() const;
    struct NCRYSTAL_API CompositionEntry {
      double fraction = -1.0;
      IndexedAtomData atom;
      CompositionEntry(double fr,IndexedAtomData);//constructor needed for C++11
    };
    typedef std::vector<CompositionEntry> Composition;
    const Composition& getComposition() const;

    /////////////////////////////////////////////////////////////////////////////
    // Display labels associated with atom data. Needs index, so that for      //
    // instance an Al atom playing two different roles in the material will be //
    // labelled "Al-a" and "Al-b" respectively.                                //
    /////////////////////////////////////////////////////////////////////////////

    const std::string& displayLabel(const AtomIndex& ai) const;

    //////////////////////////////////////////////////////
    // All AtomData instances connected to object, by   //
    // index (allows efficient AtomDataSP<->index map.  //
    // (this is used to build the C/Python interfaces). //
    //////////////////////////////////////////////////////

    AtomDataSP atomDataSP( const AtomIndex& ai ) const;
    const AtomData& atomData( const AtomIndex& ai ) const;
    IndexedAtomData indexedAtomData( const AtomIndex& ai ) const;

    /////////////////////////////////////////////////////////////////////////////
    // Custom information for which the core NCrystal code does not have any   //
    // specific treatment. This is primarily intended as a place to put extra  //
    // data needed while developing new physics models. The core NCrystal code //
    // should never make use of such custom data.                              //
    /////////////////////////////////////////////////////////////////////////////

    //Data is stored as an ordered list of named "sections", with each section
    //containing "lines" which can consist of 1 or more "words" (strings).
    typedef VectS CustomLine;
    typedef std::vector<CustomLine> CustomSectionData;
    typedef std::string CustomSectionName;
    typedef std::vector<std::pair<CustomSectionName,CustomSectionData>> CustomData;
    const CustomData& getAllCustomSections() const;

    //Convenience (count/access specific section):
    unsigned countCustomSections(const CustomSectionName& sectionname ) const;
    const CustomSectionData& getCustomSection( const CustomSectionName& name,
                                               unsigned index=0 ) const;

    //////////////////////////////
    // Internals follow here... //
    //////////////////////////////

  public:
    //Methods used by factories when setting up an Info object:
    Info() = default;
    Info( const Info& ) = delete;
    Info& operator=( const Info& ) = delete;
    Info( Info&& ) = default;
    Info& operator=( Info&& ) = default;

    void addAtom(AtomInfo&& ai) {ensureNoLock(); m_atomlist.push_back(std::move(ai)); }
    void enableHKLInfo(double dlower, double dupper);
    void addHKL(HKLInfo&& hi) { ensureNoLock(); m_hkllist.emplace_back(std::move(hi)); }
    void setHKLList(HKLList&& hkllist) { ensureNoLock(); m_hkllist = std::move(hkllist); }
    void setStructInfo(const StructureInfo& si) { ensureNoLock(); nc_assert_always(!m_structinfo.has_value()); m_structinfo = si; }
    void setXSectFree(SigmaFree x) { ensureNoLock(); m_xsect_free = x; }
    void setXSectAbsorption(SigmaAbsorption x) { ensureNoLock(); m_xsect_absorption = x; }
    void setTemperature(Temperature t) { ensureNoLock(); m_temp = t; }
    void setDensity(Density d) { ensureNoLock(); m_density = d; }
    void setNumberDensity(NumberDensity d) { ensureNoLock(); m_numberdensity = d; }
    void setXSectProvider(std::function<CrossSect(NeutronEnergy)> xsp) { ensureNoLock(); nc_assert(!!xsp); m_xsectprovider = std::move(xsp); }
    void addDynInfo(std::unique_ptr<DynamicInfo> di) { ensureNoLock(); nc_assert(di); m_dyninfolist.push_back(std::move(di)); }
    void setComposition(Composition&& c) { ensureNoLock(); m_composition = std::move(c); }
    void setCustomData(CustomData&& cd) { ensureNoLock(); m_custom = std::move(cd); }

    void objectDone();//Finish up (sorts hkl list (by dspacing first), and atom info list (by Z first)). This locks the instance.
    bool isLocked() const { return m_lock; }

  private:
    void ensureNoLock();
    UniqueID m_uid;
    Optional<StructureInfo> m_structinfo;
    AtomInfoList m_atomlist;
    HKLList m_hkllist;//sorted by dspacing first
    DynamicInfoList m_dyninfolist;
    Optional<PairDD> m_hkl_dlower_and_dupper;
    Optional<Density> m_density;
    Optional<NumberDensity> m_numberdensity;
    Optional<SigmaFree> m_xsect_free;
    Optional<SigmaAbsorption> m_xsect_absorption;
    Optional<Temperature> m_temp;
    std::function<CrossSect(NeutronEnergy)> m_xsectprovider;
    Composition m_composition;
    CustomData m_custom;
    bool m_lock = false;
    std::vector<AtomDataSP> m_atomDataSPs;
    VectS m_displayLabels;
    static void throwObsoleteDebyeTemp()
    {
      NCRYSTAL_THROW(LogicError,"The concept of global versus per-element Debye temperatures"
                     " has been removed. Revisit NCInfo.hh to see how to get Debye temperatures"
                     " from the AtomInfo objects (and note the new hasAtomDebyeTemp() method).");
    }
  public:
    //Obsolete functions which will be removed in a future release:
    bool hasAtomPositions() const { return hasAtomInfo(); }//AtomInfo objects now always have positions
    //The concept of global vs. per-element Debye temperatures have been removed. Debye temperatures now always reside on AtomInfo objects.
    bool hasAnyDebyeTemperature() const { return hasAtomDebyeTemp(); };
    //    bool hasGlobalDebyeTemperature() const { throwObsoleteDebyeTemp(); return false; }
    DebyeTemperature getGlobalDebyeTemperature() const { throwObsoleteDebyeTemp(); return DebyeTemperature{-1.0}; }
    bool hasPerElementDebyeTemperature() const { throwObsoleteDebyeTemp(); return hasAtomDebyeTemp(); }
    DebyeTemperature getDebyeTemperatureByElement(const AtomIndex&) const { throwObsoleteDebyeTemp(); return DebyeTemperature{-1.0}; }
  };

  using InfoPtr = shared_obj<const Info>;
  using OptionalInfoPtr = optional_shared_obj<const Info>;
}


////////////////////////////
// Inline implementations //
////////////////////////////

namespace NCrystal {
  inline bool Info::isCrystalline() const { return hasStructureInfo() || hasAtomPositions() || hasHKLInfo(); }
  inline bool Info::hasStructureInfo() const { return m_structinfo.has_value(); }
  inline const StructureInfo& Info::getStructureInfo() const  { nc_assert(hasStructureInfo()); return m_structinfo.value(); }
  inline bool Info::hasXSectAbsorption() const { return m_xsect_absorption.has_value(); }
  inline bool Info::hasXSectFree() const { return m_xsect_free.has_value(); }
  inline SigmaAbsorption Info::getXSectAbsorption() const { nc_assert(hasXSectAbsorption()); return m_xsect_absorption.value(); }
  inline SigmaFree Info::getXSectFree() const { nc_assert(hasXSectFree()); return m_xsect_free.value(); }
  inline bool Info::providesNonBraggXSects() const { return !!m_xsectprovider; }
  inline CrossSect Info::xsectScatNonBragg(NeutronEnergy ekin) const  { nc_assert(!!m_xsectprovider); return m_xsectprovider(ekin); }
  inline bool Info::hasTemperature() const { return m_temp.has_value(); }
  inline const AtomInfoList& Info::getAtomInfos() const { return m_atomlist; }
  inline Temperature Info::getTemperature() const { nc_assert(hasTemperature()); return m_temp.value(); }
  inline bool Info::hasAtomMSD() const { return hasAtomInfo() && m_atomlist.front().msd().has_value(); }
  inline bool Info::hasAtomDebyeTemp() const { return hasAtomInfo() && m_atomlist.front().debyeTemp().has_value(); }
  inline bool Info::hasAtomInfo() const  { return !m_atomlist.empty(); }
  inline AtomInfoList::const_iterator Info::atomInfoBegin() const { return m_atomlist.begin(); }
  inline AtomInfoList::const_iterator Info::atomInfoEnd() const { return m_atomlist.end(); }
  inline bool Info::hasHKLInfo() const { return m_hkl_dlower_and_dupper.has_value(); }
  inline const HKLList& Info::hklList() const { return m_hkllist; }
  inline bool Info::hasExpandedHKLInfo() const { return hasHKLInfo() && !m_hkllist.empty() && m_hkllist.front().eqv_hkl; }
  inline bool Info::hasHKLDemiNormals() const { return hasHKLInfo() && !m_hkllist.empty() && ! m_hkllist.front().demi_normals.empty(); }
  inline unsigned Info::nHKL() const { return m_hkllist.size(); }
  inline HKLList::const_iterator Info::hklBegin() const { return m_hkllist.begin(); }
  inline HKLList::const_iterator Info::hklLast() const
  {
    return m_hkllist.empty() ? m_hkllist.end() : std::prev(m_hkllist.end());
  }
  inline HKLList::const_iterator Info::hklEnd() const { return m_hkllist.end(); }
  inline double Info::hklDLower() const { nc_assert(hasHKLInfo()); return m_hkl_dlower_and_dupper.value().first; }
  inline double Info::hklDUpper() const { nc_assert(hasHKLInfo()); return m_hkl_dlower_and_dupper.value().second; }
  inline bool Info::hasDensity() const { return m_density.has_value(); }
  inline Density Info::getDensity() const { nc_assert(hasDensity()); return m_density.value(); }
  inline bool Info::hasNumberDensity() const { return m_numberdensity.has_value(); }
  inline NumberDensity Info::getNumberDensity() const { nc_assert(hasNumberDensity()); return m_numberdensity.value(); }
  inline bool Info::hasDynamicInfo() const { return !m_dyninfolist.empty(); }
  inline const DynamicInfoList& Info::getDynamicInfoList() const  { return m_dyninfolist; }
  inline double DynamicInfo::fraction() const { return m_fraction; }
  inline Temperature DynamicInfo::temperature() const { return m_temperature; }
  inline bool Info::hasComposition() const { return !m_composition.empty(); }
  inline const Info::Composition& Info::getComposition() const { return m_composition; }
  inline DI_VDOSDebye::DI_VDOSDebye( double fr, IndexedAtomData atom, Temperature tt, DebyeTemperature dt )
    : DI_ScatKnl(fr,std::move(atom),tt),m_dt(dt) { nc_assert(m_dt.get()>0.0); }
  inline DebyeTemperature DI_VDOSDebye::debyeTemperature() const { return m_dt; }
  inline const Info::CustomData& Info::getAllCustomSections() const { return m_custom; }
  inline const AtomData& IndexedAtomData::data() const ncnoexceptndebug { return atomDataSP; }
  inline bool IndexedAtomData::operator<(const IndexedAtomData& o) const ncnoexceptndebug {
    //Sanity check (same index means same AtomData instance):
    nc_assert( atomDataSP == o.atomDataSP || index != o.index );
    return index < o.index;
  }
  inline bool IndexedAtomData::operator==(const IndexedAtomData& o) const ncnoexceptndebug {
    //Sanity check (same index means same AtomData instance):
    nc_assert( atomDataSP == o.atomDataSP || index != o.index );
    return index == o.index;
  }
  inline const std::string& Info::displayLabel(const AtomIndex& ai) const
  {
    nc_assert(ai.get()<m_displayLabels.size());
    return m_displayLabels[ai.get()];
  }

  inline AtomDataSP Info::atomDataSP( const AtomIndex& ai ) const
  {
    nc_assert( ai.get() < m_atomDataSPs.size());
    return m_atomDataSPs[ai.get()];
  }

  inline const AtomData& Info::atomData( const AtomIndex& ai ) const
  {
    nc_assert( ai.get() < m_atomDataSPs.size());
    return m_atomDataSPs[ai.get()];
  }

  inline IndexedAtomData Info::indexedAtomData( const AtomIndex& ai ) const
  {
    nc_assert( ai.get() < m_atomDataSPs.size());
    return IndexedAtomData{m_atomDataSPs[ai.get()],{ai}};
  }
  inline Info::CompositionEntry::CompositionEntry(double fr,IndexedAtomData iad)
    : fraction(fr), atom(std::move(iad))
  {
  }
}

#endif
