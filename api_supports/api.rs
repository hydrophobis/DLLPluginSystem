//[lib]
//crate-type = ["cdylib"]
//cargo build --release

#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use std::ffi::c_char;
use std::ptr;

pub const ABI_V1: u32 = 1;

pub const PRIORITY_DEFAULT: i8 = 1;
pub const PRIORITY_FIRST: i8 = 0;
pub const PRIORITY_LATER: i8 = 2;

pub const DEP_TYPE_REQUIRED: u8 = 0;
pub const DEP_TYPE_OPTIONAL: u8 = 1;

pub type event_callback_t = extern "C" fn(*const c_char, *const c_char);

#[repr(C)]
#[derive(Copy, Clone)]
pub struct Dependency {
    pub name: *const c_char,
    pub r#type: u8,
}

#[repr(C)]
pub struct PluginInfo {
    pub name: *const c_char,
    pub version: *const c_char,
    pub abi_version: u32,
    pub priority: i8,
    pub _pad: [u8; 3],
    pub dependencies: [Dependency; 128],
}

#[repr(C)]
pub struct PluginHost {
    pub send_event: extern "C" fn(*const c_char, *const c_char),
    pub register_event: extern "C" fn(*const c_char, event_callback_t),
    pub unregister_event: extern "C" fn(event_callback_t),

    pub load_plugin: extern "C" fn(*const c_char) -> bool,
    pub unload_plugin: extern "C" fn(*const c_char) -> bool,

    pub log: extern "C" fn(*const c_char, *const c_char),

    pub set_data: extern "C" fn(*const c_char, *const c_char) -> bool,
    pub get_data: extern "C" fn(*const c_char) -> *const c_char,
    pub has_data: extern "C" fn(*const c_char) -> bool,
    pub delete_data: extern "C" fn(*const c_char) -> bool,

    pub set_timer: extern "C" fn(u32, event_callback_t, bool) -> u64,
    pub cancel_timer: extern "C" fn(u64) -> bool,
}

static mut HOST: *mut PluginHost = ptr::null_mut();

#[no_mangle]
pub extern "C" fn plugin_init(host: *mut PluginHost) -> bool {
    unsafe { HOST = host; }
    true
}

#[no_mangle]
pub extern "C" fn plugin_shutdown() {}

static mut INFO: PluginInfo = PluginInfo {
    name: b"ExamplePlugin\0".as_ptr() as _,
    version: b"1.0.0\0".as_ptr() as _,
    abi_version: ABI_V1,
    priority: PRIORITY_DEFAULT,
    _pad: [0; 3],
    dependencies: [Dependency { name: ptr::null(), r#type: 0 }; 128],
};

#[no_mangle]
pub extern "C" fn plugin_get_info() -> *const PluginInfo {
    unsafe { &INFO }
}

/* ---------------- Helper API ---------------- */

pub mod plugin {
    use super::*;

    pub unsafe fn send(evt: *const c_char, payload: *const c_char) {
        if !super::HOST.is_null() {
            ((*super::HOST).send_event)(evt, payload);
        }
    }

    pub unsafe fn log(level: *const c_char, msg: *const c_char) {
        if !super::HOST.is_null() {
            ((*super::HOST).log)(level, msg);
        }
    }

    pub unsafe fn store(key: *const c_char, value: *const c_char) -> bool {
        if super::HOST.is_null() { false } else { ((*super::HOST).set_data)(key, value) }
    }

    pub unsafe fn load(key: *const c_char) -> *const c_char {
        if super::HOST.is_null() { ptr::null() } else { ((*super::HOST).get_data)(key) }
    }

    pub unsafe fn timer(ms: u32, cb: event_callback_t, repeat: bool) -> u64 {
        if super::HOST.is_null() { 0 } else { ((*super::HOST).set_timer)(ms, cb, repeat) }
    }

    pub unsafe fn on(evt: *const c_char, cb: event_callback_t) {
        if !super::HOST.is_null() {
            ((*super::HOST).register_event)(evt, cb);
        }
    }

    pub unsafe fn off(cb: event_callback_t) {
        if !super::HOST.is_null() {
            ((*super::HOST).unregister_event)(cb);
        }
    }
}
