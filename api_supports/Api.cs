// dotnet publish -c Release -r win-x64 /p:NativeLib=Shared
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

public static class PluginConstants
{
    public const uint ABI_V1 = 1;

    public const sbyte PRIORITY_DEFAULT = 1;
    public const sbyte PRIORITY_FIRST = 0;
    public const sbyte PRIORITY_LATER = 2;

    public const byte DEP_TYPE_REQUIRED = 0;
    public const byte DEP_TYPE_OPTIONAL = 1;
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct Dependency
{
    public sbyte* name;
    public byte type;
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct PluginInfo
{
    public sbyte* name;
    public sbyte* version;
    public uint abi_version;
    public sbyte priority;
    private fixed byte _pad[3];
    public fixed Dependency dependencies[128];
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public unsafe delegate void EventCallback(sbyte* eventName, sbyte* payload);

[StructLayout(LayoutKind.Sequential)]
public unsafe struct PluginHost
{
    public delegate* unmanaged[Cdecl]<sbyte*, sbyte*, void> send_event;
    public delegate* unmanaged[Cdecl]<sbyte*, EventCallback, void> register_event;
    public delegate* unmanaged[Cdecl]<EventCallback, void> unregister_event;

    public delegate* unmanaged[Cdecl]<sbyte*, bool> load_plugin;
    public delegate* unmanaged[Cdecl]<sbyte*, bool> unload_plugin;

    public delegate* unmanaged[Cdecl]<sbyte*, sbyte*, void> log;

    public delegate* unmanaged[Cdecl]<sbyte*, sbyte*, bool> set_data;
    public delegate* unmanaged[Cdecl]<sbyte*, sbyte*> get_data;
    public delegate* unmanaged[Cdecl]<sbyte*, bool> has_data;
    public delegate* unmanaged[Cdecl]<sbyte*, bool> delete_data;

    public delegate* unmanaged[Cdecl]<uint, EventCallback, bool, ulong> set_timer;
    public delegate* unmanaged[Cdecl]<ulong, bool> cancel_timer;
}

public unsafe static class Plugin
{
    public static PluginHost* Host;

    public static void Send(sbyte* evt, sbyte* payload = null)
        => Host->send_event(evt, payload);

    public static void Log(sbyte* level, sbyte* msg)
        => Host->log(level, msg);

    public static bool Store(sbyte* key, sbyte* value)
        => Host->set_data(key, value);

    public static sbyte* Load(sbyte* key)
        => Host->get_data(key);

    public static ulong Timer(uint ms, EventCallback cb, bool repeat = false)
        => Host->set_timer(ms, cb, repeat);

    public static void On(sbyte* evt, EventCallback cb)
        => Host->register_event(evt, cb);

    public static void Off(EventCallback cb)
        => Host->unregister_event(cb);
}

public unsafe static class PluginExports
{
    static PluginInfo info;

    static PluginExports()
    {
        info = new PluginInfo
        {
            name = (sbyte*)Marshal.StringToHGlobalAnsi("ExamplePlugin"),
            version = (sbyte*)Marshal.StringToHGlobalAnsi("1.0.0"),
            abi_version = PluginConstants.ABI_V1,
            priority = PluginConstants.PRIORITY_DEFAULT
        };
    }

    [UnmanagedCallersOnly(EntryPoint = "plugin_get_info", CallConvs = new[] { typeof(CallConvCdecl) })]
    public static PluginInfo* GetInfo() => &info;

    [UnmanagedCallersOnly(EntryPoint = "plugin_init", CallConvs = new[] { typeof(CallConvCdecl) })]
    public static bool Init(PluginHost* host)
    {
        Plugin.Host = host;
        return true;
    }

    [UnmanagedCallersOnly(EntryPoint = "plugin_shutdown", CallConvs = new[] { typeof(CallConvCdecl) })]
    public static void Shutdown() { }
}
