
#include <iostream>

#include <ert/util/test_util.hpp>

#include <nexus/unit.hpp>


void test_field_units() {
    const nex::UnitSystem u( nex::UnitSystem::UnitType::field );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::acoustic_impedance).c_str(),     "(ft/sec)(g/cm3)" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::acoustic_wave_velocity).c_str(), "ft/sec" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::angle).c_str(),                  "degrees" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::area).c_str(),                   "ft2" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::bulk_modulus).c_str(),           "psia" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::transmissibility).c_str(),       "ft3 cp/day/psi" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::viscosity).c_str(),              "cp" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::volume).c_str(),                 "ft3" );
}

void test_lab_units() {
    const nex::UnitSystem u( nex::UnitSystem::UnitType::lab );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::compressibility).c_str(),             "psi-1" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::critial_pressure).c_str(),            "psia" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::density).c_str(),                     "gm/cc" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::formation_volume_factor_gas).c_str(), "cc/stcc" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::formation_volume_factor_oil).c_str(), "cc/stcc" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::transmissibility).c_str(),            "cc cp/hour/psi" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::viscosity).c_str(),                   "cp" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::volume).c_str(),                      "cc" );
}

void test_metric_kPa_units() {
    const nex::UnitSystem u( nex::UnitSystem::UnitType::metric_kPa );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::fraction).c_str(),         "fraction" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::gas_liquid_ratio).c_str(), "SM3/STM3" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::gravity_gradient).c_str(), "kPa/m" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::length).c_str(),           "m" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::moles).c_str(),            "kg-moles" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::transmissibility).c_str(), "m3 cp/day/kPa" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::viscosity).c_str(),        "cp" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::volume).c_str(),           "m3" );
}

void test_metric_kg_cm2_units() {
    const nex::UnitSystem u( nex::UnitSystem::UnitType::metric_kg_cm2 );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::molar_density).c_str(),    "kg-moles/m3" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::molar_rates).c_str(),      "kg-moles/day" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::permeability).c_str(),     "md" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::pressure).c_str(),         "kg/cm2" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::reservoir_rates).c_str(),  "m3/day" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::transmissibility).c_str(), "m3 cp/day/kg/cm2" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::viscosity).c_str(),        "cp" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::volume).c_str(),           "m3" );
}

void test_metric_bars_units() {
    const nex::UnitSystem u( nex::UnitSystem::UnitType::metric_bars );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::reservoir_volumes).c_str(),    "km3" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::saturation).c_str(),           "fraction" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::surface_rates_gas).c_str(),    "SM3/day" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::surface_rates_liquid).c_str(), "STM3/day" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::surface_volumes_gas).c_str(),  "kSTM3" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::transmissibility).c_str(),     "m3 cp/day/bars" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::viscosity).c_str(),            "cp" );
    test_assert_string_equal( u.unit_str(nex::UnitSystem::Measure::volume).c_str(),               "m3" );
}


int main(int argc, char **argv) {
    test_field_units();
    test_lab_units();
    test_metric_kPa_units();
    test_metric_kg_cm2_units();
    test_metric_bars_units();
    exit(0);
}
