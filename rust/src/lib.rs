// lib.rs - the Oplus charger HAL, in Rust.
//
// This is a behaviour-preserving implementation of the C++ HAL in ../src and
// ../include. Node paths, fallback order, sentinel values and timing
// constants match the C++ version. The two files that would otherwise drift
// by transcription (rust/src/charger/accessors.rs and rust/src/txn.rs) are
// produced from the C++ sources by rust/gen_accessors.py and rust/gen_txn.py.

pub mod binder;
pub mod bypass;
pub mod charger;
pub mod constants;
pub mod decimal;
pub mod detection;
pub mod fastcharge;
pub mod log;
pub mod monitor;
pub mod nodes;
pub mod props;
pub mod testkit;
pub mod txn;

pub use charger::Charger;

#[cfg(test)]
mod port_tests;
