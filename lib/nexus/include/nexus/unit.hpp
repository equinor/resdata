
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

        enum class Measure : int {
            acoustic_impedance,
            acoustic_wave_velocity,
            angle,
            area,
            bulk_modulus,
            compressibility,
            critial_pressure,
            density,
            formation_volume_factor_gas,
            formation_volume_factor_oil,
            fraction,
            gas_liquid_ratio,
            gravity_gradient,
            length,
            moles,
            molar_density,
            molar_rates,
            permeability,
            pressure,
            reservoir_rates,
            reservoir_volumes,
            saturation,
            surface_rates_gas,
            surface_rates_liquid,
            surface_volumes_gas,
            surface_volumes_liquid,
            temperature,
            time,
            tracer_consentrations,
            transmissibility,
            viscosity,
            volume
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
