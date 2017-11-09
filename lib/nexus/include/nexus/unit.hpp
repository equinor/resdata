
#include <string>
#include <array>

namespace nex {

    class UnitSystem {
    public:

        enum class UnitType : int {
            field,
            lab,
            metric_kPa,
            metric_kg_cm2,
            metric_bars
        };


        /**
         * This list of measures are the measures supported by nexus and might
         * be incomplete for ecl. The eclipse unit for the measures that are
         * commented out is unknown. The units for reservoir_volumes,
         * surface_volumes_gas, and surface_volumes_liquid are given in k SM3 in
         * nexus, while they are just SM3 in eclipse. We carry this k over to
         * the eclipse output.
         */
        enum class Measure : int {
            // acoustic_impedance,
            // acoustic_wave_velocity,
            // angle,
            // area,
            // bulk_modulus,
            compressibility,
            concentration,
            density,
            formation_volume_factor_gas,
            formation_volume_factor_oil,
            fraction,
            gas_liquid_ratio,
            // gravity_gradient,
            identity,
            length,
            moles,
            // molar_density,
            // molar_rates,
            permeability,
            pressure,
            pressure_absolute,
            reservoir_rates,
            reservoir_volumes,
            // saturation,
            surface_rates_gas,
            surface_rates_liquid,
            surface_volumes_gas,
            surface_volumes_liquid,
            temperature,
            time,
            // tracer_consentrations,
            // transmissibility,
            viscosity,
            volume,
            water_cut
        };

        UnitSystem( std::array< char, 6 > );
        UnitSystem( std::string );
        UnitSystem( UnitType );

        std::string unit_str( Measure ) const;
        std::string unit_str( const std::string& ) const;

    private:
        UnitType unit;
        const char* const* unit_str_table;

    };

} /* nex */
