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
    /* concentration               */ "KG/SM3",
    /* density                     */ "KG/M3",
    /* formation_volume_factor_gas */ "RM3/SM3",
    /* formation_volume_factor_oil */ "RM3/SM3",
    /* fraction                    */ "",
    /* gas_liquid_ratio            */ "SM3/SM3",
    // /* gravity_gradient            */ "BARS/M",
    /* identity                    */ "",
    /* length                      */ "M",
    /* moles                       */ "KG-M",
    // /* molar_density               */ "KG-MOLES/M3",
    // /* molar_rates                 */ "KG-MOLES/DAY",
    /* permeability                */ "MD",
    /* pressure                    */ "BARS",
    /* pressure_absolute           */ "BARSA",
    /* reservoir_rates             */ "RM3/DAY",
    /* reservoir_volumes           */ "RM3",
    // /* saturation                  */ "FRACTION",
    /* surface_rates_gas           */ "SM3/DAY",
    /* surface_rates_liquid        */ "SM3/DAY",
    /* surface_volumes_gas         */ "SM3",
    /* surface_volumes_liquid      */ "SM3",
    /* temperature                 */ "C",
    /* time                        */ "DAY",
    // /* tracer_consentrations       */ "FRACTION",
    // /* transmissibility            */ "M3 CP/DAY/BARS",
    /* viscosity                   */ "CP",
    /* volum                       */ "M3",
    /* water_cut                   */ "SM3/SM3"
};

static const constexpr float conversion_table
[static_cast<int>( UnitSystem::Measure::measure_enum_size )]
[static_cast<int>( UnitSystem::UnitType::unit_type_count )] = {
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* compressibility             */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* concentration               */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* density                     */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* formation_volume_factor_gas */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* formation_volume_factor_oil */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* fraction                    */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* gas_liquid_ratio            */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* identity                    */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* length                      */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* moles                       */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* permeability                */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* pressure                    */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* pressure_absolute           */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* reservoir_rates             */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* reservoir_volumes           */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* surface_rates_gas           */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* surface_rates_liquid        */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* surface_volumes_gas         */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* surface_volumes_liquid      */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* temperature                 */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* time                        */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* viscosity                   */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }, /* volum                       */
    { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f    }  /* water_cut                   */
};

static const std::map< std::string, UnitSystem::Measure >
varname_to_measure = {
    {"QOP" , UnitSystem::Measure::surface_rates_liquid   },
    {"QWP" , UnitSystem::Measure::surface_rates_liquid   },
    {"QGP" , UnitSystem::Measure::surface_rates_gas      },
    {"GOR" , UnitSystem::Measure::gas_liquid_ratio       },
    {"WCUT", UnitSystem::Measure::water_cut              },
    {"COP" , UnitSystem::Measure::surface_volumes_liquid },
    {"CWP" , UnitSystem::Measure::surface_volumes_liquid },
    {"CGP" , UnitSystem::Measure::surface_volumes_gas    },
    {"QWI" , UnitSystem::Measure::surface_rates_liquid   },
    {"QGI" , UnitSystem::Measure::surface_rates_gas      },
    {"CWI" , UnitSystem::Measure::surface_volumes_liquid },
    {"CGI" , UnitSystem::Measure::surface_volumes_gas    },
    {"QPP" , UnitSystem::Measure::surface_rates_liquid   },
    {"CPP" , UnitSystem::Measure::concentration          },
    {"COWP", UnitSystem::Measure::surface_volumes_liquid },
    {"QOWP", UnitSystem::Measure::surface_rates_liquid   },
    {"GOR" , UnitSystem::Measure::gas_liquid_ratio       },
    {"PRDW", UnitSystem::Measure::identity               },
    {"CCPP", UnitSystem::Measure::surface_volumes_liquid },
    {"CCPI", UnitSystem::Measure::surface_volumes_liquid },
    {"QPI" , UnitSystem::Measure::surface_rates_liquid   },
    {"CPI" , UnitSystem::Measure::concentration          },
    {"BHP" , UnitSystem::Measure::pressure               },
    {"THP" , UnitSystem::Measure::pressure               },
    {"WPAV", UnitSystem::Measure::pressure               },
    {"OIP" , UnitSystem::Measure::surface_volumes_liquid },
    {"WIP" , UnitSystem::Measure::surface_volumes_liquid },
    {"GIP" , UnitSystem::Measure::surface_volumes_gas    },
    {"PAVH", UnitSystem::Measure::pressure               }
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

UnitSystem::Measure UnitSystem::measure( const std::string& varname ) const {
    auto it = varname_to_measure.find( varname );
    if ( it != varname_to_measure.end() )
        return it->second;
    std::cerr << "Warning: no unit found for nexus variable " << varname
              << std::endl;
    return Measure::identity;
}

float UnitSystem::conversion( Measure m ) const {
    int ui = static_cast<int>( this->unit );
    int mi = static_cast<int>( m );
    return conversion_table[mi][ui];
}

float UnitSystem::conversion( const std::string& varname ) const {
    return this->conversion( this->measure(varname) );
}

std::string UnitSystem::unit_str( Measure m ) const {
    return this->unit_str_table[ static_cast<int>(m) ];
}

std::string UnitSystem::unit_str( const std::string& varname ) const {
    return this->unit_str( this->measure(varname) );
}


} // nex
