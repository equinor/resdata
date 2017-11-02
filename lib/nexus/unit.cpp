#include <iostream>
#include <map>
#include <stdexcept>

#include <nexus/unit.hpp>

namespace nex {


namespace {

static const constexpr char* field_table[] = {
    /* acoustic_impedance          */ "(ft/sec)(g/cm3)",
    /* acoustic_wave_velocity      */ "ft/sec",
    /* angle                       */ "degrees",
    /* area                        */ "ft2",
    /* bulk_modulus                */ "psia",
    /* compressibility             */ "psi-1",
    /* critial_pressure            */ "psia",
    /* density                     */ "lb/ft3",
    /* formation_volume_factor_gas */ "RB/STB",
    /* formation_volume_factor_oil */ "RB/MSCF",
    /* fraction                    */ "fraction",
    /* gas_liquid_ratio            */ "MSCF/STB",
    /* gravity_gradient            */ "psi/ft",
    /* length                      */ "ft",
    /* moles                       */ "lb-moles",
    /* molar_density               */ "lb-moles/ft3",
    /* molar_rates                 */ "lb-moles/day",
    /* permeability                */ "md",
    /* pressure                    */ "psia",
    /* reservoir_rates             */ "RB/day",
    /* reservoir_volumes           */ "MRB",
    /* saturation                  */ "fraction",
    /* surface_rates_gas           */ "MSCF/day",
    /* surface_rates_liquid        */ "STB/day",
    /* surface_volumes_gas         */ "MMSCF",
    /* surface_volumes_liquid      */ "MSTB",
    /* temperature                 */ "F",
    /* time                        */ "days",
    /* tracer_consentrations       */ "fraction",
    /* transmissibility            */ "ft3 cp/day/psi",
    /* viscosity                   */ "cp",
    /* volum                       */ "ft3"
};

static const constexpr char* lab_table[] = {
    /* acoustic_impedance          */ "(cm/sec)(g/cm3)",
    /* acoustic_wave_velocity      */ "m/sec",
    /* angle                       */ "degrees",
    /* area                        */ "cm2",
    /* bulk_modulus                */ "psia",
    /* compressibility             */ "psi-1",
    /* critial_pressure            */ "psia",
    /* density                     */ "gm/cc",
    /* formation_volume_factor_gas */ "cc/stcc",
    /* formation_volume_factor_oil */ "cc/stcc",
    /* fraction                    */ "fraction",
    /* gas_liquid_ratio            */ "stcc/stcc",
    /* gravity_gradient            */ "psi/cm",
    /* length                      */ "cm",
    /* moles                       */ "gm-moles",
    /* molar_density               */ "gm-moles/cm3",
    /* molar_rates                 */ "gm-moles/hour",
    /* permeability                */ "md",
    /* pressure                    */ "psia",
    /* reservoir_rates             */ "cc/hour",
    /* reservoir_volumes           */ "kcc",
    /* saturation                  */ "fraction",
    /* surface_rates_gas           */ "stcc/hour",
    /* surface_rates_liquid        */ "stcc/hour",
    /* surface_volumes_gas         */ "kstcc",
    /* surface_volumes_liquid      */ "kstcc",
    /* temperature                 */ "C",
    /* time                        */ "hours",
    /* tracer_consentrations       */ "fraction",
    /* transmissibility            */ "cc cp/hour/psi",
    /* viscosity                   */ "cp",
    /* volum                       */ "cc"
};

static const constexpr char* metric_kPa_table[] = {
    /* acoustic_impedance          */ "(m/sec)(kg/m3)",
    /* acoustic_wave_velocity      */ "m/sec",
    /* angle                       */ "degrees",
    /* area                        */ "m2",
    /* bulk_modulus                */ "kPa",
    /* compressibility             */ "kPa-1",
    /* critial_pressure            */ "kPaa",
    /* density                     */ "kg/m3",
    /* formation_volume_factor_gas */ "m3/STM3",
    /* formation_volume_factor_oil */ "m3/STM3",
    /* fraction                    */ "fraction",
    /* gas_liquid_ratio            */ "SM3/STM3",
    /* gravity_gradient            */ "kPa/m",
    /* length                      */ "m",
    /* moles                       */ "kg-moles",
    /* molar_density               */ "kg-moles/m3",
    /* molar_rates                 */ "kg-moles/day",
    /* permeability                */ "md",
    /* pressure                    */ "kPa",
    /* reservoir_rates             */ "m3/day",
    /* reservoir_volumes           */ "km3",
    /* saturation                  */ "fraction",
    /* surface_rates_gas           */ "SM3/day",
    /* surface_rates_liquid        */ "STM3/day",
    /* surface_volumes_gas         */ "kSTM3",
    /* surface_volumes_liquid      */ "kSTM3",
    /* temperature                 */ "C",
    /* time                        */ "days",
    /* tracer_consentrations       */ "fraction",
    /* transmissibility            */ "m3 cp/day/kPa",
    /* viscosity                   */ "cp",
    /* volum                       */ "m3"
};

static const constexpr char* metric_kg_cm2_table[] = {
    /* acoustic_impedance          */ "(m/sec)(kg/m3)",
    /* acoustic_wave_velocity      */ "m/sec",
    /* angle                       */ "degrees",
    /* area                        */ "m2",
    /* bulk_modulus                */ "kg/cm2",
    /* compressibility             */ "(kg/cm2)-1",
    /* critial_pressure            */ "(kg/cm2)a",
    /* density                     */ "kg/m3",
    /* formation_volume_factor_gas */ "m3/STM3",
    /* formation_volume_factor_oil */ "m3/STM3",
    /* fraction                    */ "fraction",
    /* gas_liquid_ratio            */ "SM3/STM3",
    /* gravity_gradient            */ "kg/cm2/m",
    /* length                      */ "m",
    /* moles                       */ "kg-moles",
    /* molar_density               */ "kg-moles/m3",
    /* molar_rates                 */ "kg-moles/day",
    /* permeability                */ "md",
    /* pressure                    */ "kg/cm2",
    /* reservoir_rates             */ "m3/day",
    /* reservoir_volumes           */ "km3",
    /* saturation                  */ "fraction",
    /* surface_rates_gas           */ "SM3/day",
    /* surface_rates_liquid        */ "STM3/day",
    /* surface_volumes_gas         */ "kSTM3",
    /* surface_volumes_liquid      */ "kSTM3",
    /* temperature                 */ "C",
    /* time                        */ "days",
    /* tracer_consentrations       */ "fraction",
    /* transmissibility            */ "m3 cp/day/kg/cm2",
    /* viscosity                   */ "cp",
    /* volum                       */ "m3"
};

static const constexpr char* metric_bars_table[] = {
    /* acoustic_impedance          */ "(m/sec)(kg/m3)",
    /* acoustic_wave_velocity      */ "m/sec",
    /* angle                       */ "degrees",
    /* area                        */ "m2",
    /* bulk_modulus                */ "bars",
    /* compressibility             */ "bars-1",
    /* critial_pressure            */ "barsa",
    /* density                     */ "kg/m3",
    /* formation_volume_factor_gas */ "m3/STM3",
    /* formation_volume_factor_oil */ "m3/STM3",
    /* fraction                    */ "fraction",
    /* gas_liquid_ratio            */ "SM3/STM3",
    /* gravity_gradient            */ "bars/m",
    /* length                      */ "m",
    /* moles                       */ "kg-moles",
    /* molar_density               */ "kg-moles/m3",
    /* molar_rates                 */ "kg-moles/day",
    /* permeability                */ "md",
    /* pressure                    */ "bars",
    /* reservoir_rates             */ "m3/day",
    /* reservoir_volumes           */ "km3",
    /* saturation                  */ "fraction",
    /* surface_rates_gas           */ "SM3/day",
    /* surface_rates_liquid        */ "STM3/day",
    /* surface_volumes_gas         */ "kSTM3",
    /* surface_volumes_liquid      */ "kSTM3",
    /* temperature                 */ "C",
    /* time                        */ "days",
    /* tracer_consentrations       */ "fraction",
    /* transmissibility            */ "m3 cp/day/bars",
    /* viscosity                   */ "cp",
    /* volum                       */ "m3"
};


static const std::map< std::string, UnitSystem::Measure >
varname_to_unit_str = {
    {"QOP" , UnitSystem::Measure::surface_rates_liquid   },
    {"QWP" , UnitSystem::Measure::surface_rates_liquid   },
    {"QGP" , UnitSystem::Measure::surface_rates_gas      },
    {"GOR" , UnitSystem::Measure::gas_liquid_ratio       },
    {"WCUT", UnitSystem::Measure::fraction               },
    {"COP" , UnitSystem::Measure::surface_volumes_liquid },
    {"CWP" , UnitSystem::Measure::surface_volumes_liquid },
    {"CGP" , UnitSystem::Measure::surface_volumes_gas    },
    {"QWI" , UnitSystem::Measure::surface_rates_liquid   },
    {"QGI" , UnitSystem::Measure::surface_rates_gas      },
    {"CWI" , UnitSystem::Measure::surface_volumes_liquid },
    {"CGI" , UnitSystem::Measure::surface_volumes_gas    },
    {"QPP" , UnitSystem::Measure::surface_rates_liquid   },
    {"CPP" , UnitSystem::Measure::surface_volumes_liquid }
};


} // anonymous namespace

UnitSystem::UnitSystem( std::array< char, 6 > unit ) :
    UnitSystem( std::string( unit.begin(), unit.end() ) ) {}

UnitSystem::UnitSystem( std::string unit ) :
    UnitSystem(
        std::equal( unit.begin(), unit.end(), "FIELD " )
            ? UnitType::field :
        std::equal( unit.begin(), unit.end(), "LAB   " )
            ? UnitType::lab :
        std::equal( unit.begin(), unit.end(), "METRIC" )
            ? UnitType::metric_kPa :
        std::equal( unit.begin(), unit.end(), "METBAR" )
            ? UnitType::metric_bars :
        std::equal( unit.begin(), unit.end(), "METKG " )
            ? UnitType::metric_kg_cm2 :
        throw std::runtime_error( "Unsupported unit system " + unit )
    )
{}

UnitSystem::UnitSystem( UnitType unit ) {
    this->unit = unit;
    switch (unit)
    {
    case UnitType::field :
        this->unit_str_table = field_table;
        break;
    case UnitType::lab :
        this->unit_str_table = lab_table;
        break;
    case UnitType::metric_kPa :
        this->unit_str_table = metric_kPa_table;
        break;
    case UnitType::metric_kg_cm2 :
        this->unit_str_table = metric_kg_cm2_table;
        break;
    case UnitType::metric_bars :
        this->unit_str_table = metric_bars_table;
        break;
    }
}

std::string UnitSystem::unit_str( Measure measure ) const {
    return this->unit_str_table[static_cast<int>(measure)];
}

std::string UnitSystem::unit_str( const std::string& varname ) const {
    auto it = varname_to_unit_str.find( varname );
    if (it != varname_to_unit_str.end())
        return this->unit_str_table[static_cast<int>(it->second)];
    std::cerr << "Warning: no unit found for nexus variable " << varname
              << std::endl;
    return "";
}


} // nex
