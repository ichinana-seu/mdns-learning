mDNS（多播 DNS，端口 5353，域名后缀.local.）完全复用标准 DNS 的资源记录类型，核心用于局域网服务发现（DNS-SD）和设备名解析。你问到的 PTR 是其中最关键的服务发现记录，下面按常用程度 + 实际用途，把 mDNS 里的核心记录一次性讲清：
一、mDNS 核心记录（服务发现必用）
1. PTR 记录（Pointer）—— 服务反向指针
作用：从「服务类型」找到「具体服务实例名」，是 DNS-SD 服务发现的入口。
mDNS 场景：局域网里查有哪些同类服务（如打印机、AirPlay、SMB）。
格式：_服务名._协议.local. IN PTR 实例名._服务名._协议.local.
示例：
_airplay._tcp.local. PTR MyTV._airplay._tcp.local.
表示：AirPlay 服务下有一个叫 MyTV 的实例。
2. SRV 记录（Service Location）—— 服务定位
作用：告诉客户端服务所在的主机名、端口、优先级 / 权重。
mDNS 场景：拿到实例名后，查它跑在哪个设备、哪个端口。
格式：实例名._服务名._协议.local. IN SRV 优先级 权重 端口 主机名.local.
示例：
MyTV._airplay._tcp.local. IN SRV 0 0 7000 MyTV.local.
表示：该 AirPlay 服务在 MyTV 设备的 7000 端口。
3. A 记录（IPv4 地址）
作用：主机名 → IPv4 地址。
mDNS 场景：把 .local. 设备名解析为局域网 IPv4。
示例：MyTV.local. IN A 192.168.1.105
4. AAAA 记录（IPv6 地址）
作用：主机名 → IPv6 地址。
mDNS 场景：IPv6 环境下的设备名解析。
示例：MyTV.local. IN AAAA fd00::c256:23ff:fe12:3456
5. TXT 记录（Text）—— 服务附加信息
作用：携带服务的元数据 / 特征参数（键值对）。
mDNS 场景：设备型号、版本、加密方式、设备 ID、功能开关等。
示例（AirPlay）：
MyTV._airplay._tcp.local. IN TXT "deviceid=12:34:56:78:9A:BC" "model=AppleTV11,1" "vers=6.0"