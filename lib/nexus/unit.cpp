#include <iostream>
#include <map>
#include <stdexcept>

#include <nexus/unit.hpp>

namespace nex {


namespace {

static const constexpr char* metric_bars_table[] = {
    // /* acoustic_impedance          */ "(m/sec)(kg/m3)",
    // /* acoustic_wave_velocity      */ "m/sec",
    // /* angle                       */ "degrees",
    // /* area                        */ "m2",
    // /* bulk_modulus                */ "BARS",
    /* compressibility             */ "BARS-1",
    /* density                     */ "KG/M3",
    /* formation_volume_factor_gas */ "RM3/SM3",
    /* formation_volume_factor_oil */ "RM3/SM3",
    /* fraction                    */ "FRACTION",
    /* gas_liquid_ratio            */ "SM3/SM3",
    /* gravity_gradient            */ "BARS/M",
    /* length                      */ "M",
    /* moles                       */ "KG-M",
    // /* molar_density               */ "KG-MOLES/M3",
    // /* molar_rates                 */ "KG-MOLES/DAY",
    /* permeability                */ "MD",
    /* pressure                    */ "BARS",
    /* pressure_absolute           */ "BARSA",
    /* reservoir_rates             */ "RM3/DAY",
    /* reservoir_volumes           */ "kRM3",
    /* saturation                  */ "FRACTION",
    /* surface_rates_gas           */ "SM3/DAY",
    /* surface_rates_liquid        */ "SM3/DAY",
    /* surface_volumes_gas         */ "kSM3",
    /* surface_volumes_liquid      */ "kSM3",
    /* temperature                 */ "C",
    /* time                        */ "DAYS",
    /* tracer_consentrations       */ "FRACTION",
    // /* transmissibility            */ "M3 CP/DAY/BARS",
    /* viscosity                   */ "CP",
    /* volum                       */ "RM3"
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
        // std::equal( unit.begin(), unit.end(), "FIELD " )
        //     ? UnitType::field :
        // std::equal( unit.begin(), unit.end(), "LAB   " )
        //     ? UnitType::lab :
        // std::equal( unit.begin(), unit.end(), "METRIC" )
        //     ? UnitType::metric_kPa :
        std::equal( unit.begin(), unit.end(), "METBAR" )
            ? UnitType::metric_bars :
        // std::equal( unit.begin(), unit.end(), "METKG " )
        //     ? UnitType::metric_kg_cm2 :
        throw std::runtime_error( "Unsupported unit system " + unit )
    )
{}

UnitSystem::UnitSystem( UnitType unit ) {
    this->unit = unit;
    switch (unit)
    {
    // case UnitType::field :
    //     this->unit_str_table = field_table;
    //     break;
    // case UnitType::lab :
    //     this->unit_str_table = lab_table;
    //     break;
    // case UnitType::metric_kPa :
    //     this->unit_str_table = metric_kPa_table;
    //     break;
    // case UnitType::metric_kg_cm2 :
    //     this->unit_str_table = metric_kg_cm2_table;
    //     break;
    case UnitType::metric_bars :
        this->unit_str_table = metric_bars_table;
        break;
    default:
        throw std::runtime_error( "Only the metric_bars unit system is currently supported." );
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
