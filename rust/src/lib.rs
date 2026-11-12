// lib.rs - the Oplus charger HAL, in Rust.
//
// This is a behaviour-preserving port of the C++ reconstruction in
// ../src and ../include.  Nothing here improves on the original: every node
// path, fallback order, sentinel value and timing constant was copied from
// it, and the two files that would otherwise drift by transcription
// (rust/src/charger/accessors.rs and rust/src/txn.rs) are generated from
// the C++ sources by rust/gen_accessors.py and rust/gen_txn.py.

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
