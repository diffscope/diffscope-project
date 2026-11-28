<!--
SPDX-FileCopyrightText: Team OpenVPI
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# DiffScope Audio Bridge Server 设计

## 1. 实施范围

本方案的实施对象为 DiffScope 内置插件 **Audio Bridge Server**。插件负责监听控制连接、建立共享内存音频端口、绑定工程窗口、调度音频拉取和转发持久化请求。每个 DAW 插件实例对应一个工程窗口、一条活动控制连接及一组独立的音频端口。客户端实现另行安排。本文规定 VST3、AUv2 和 LV2 客户端共用的服务端协议及宿主输入约束。

插件 target 为 `audiobridgeserver`，插件 ID 为 `org.diffscope.audiobridgeserver`。DiffScope 构建始终注册该插件，并要求 iceoryx2 0.10.0 的 CMake 包。插件使用 `diffscope_add_builtin_plugin`，插件依赖仅为 Core 和 Audio。插件链接 Qt Network、DspxProject、共享协议子模块及 iceoryx2 C++ 静态绑定。初始化时创建服务对象，在 `extensionsInitialized()` 开始监听。所有对其他插件的调用使用公开入口。

| 场景 | 位置来源 | 输出 | 未完成的合成片段 |
| --- | --- | --- | --- |
| DAW 实时播放 | DAW 的工程采样位置 | 发出请求的 DAW 回调 | 缺失片段静音，其他轨道继续输出 |
| DAW 离线渲染 | DAW 的工程采样位置 | 发出请求的 DAW 离线回调 | 全工程的渲染准备成功后输出 |
| DAW 停止走带且继续处理插件时的试听 | DiffScope 工程走带，按已渲染的帧数推进 | 发出请求的 DAW 回调 | 当前区段未就绪时输出静音并保持走带位置 |

每个 Bridge 实例将一个 DiffScope 工程的 `float32` 音频送入 DAW 插件输出。VST3、AUv2 和 LV2 客户端均声明 16 个立体声输出，总计 32 个声道。Bridge 复用工程音频设备输出使用的混音器和「多声道输出」设置。关闭该设置时，工程总混音使用第一对声道。开启时，混音器按工程轨道顺序分配立体声对，左声道在前。轨道重排后，分配随工程顺序更新。混音器清零未使用的声道。协议支持其他输出声道数。单声道渲染取混音器前两路输出的平均值。 [工程输出设置](src/plugins/audio/internal/addon/ProjectAudioAddOn.cpp#L477) · [音频会话配置与读块](src/plugins/audio/audiocontext/ProjectAudioContext.cpp#L257)

采样位置为有符号值，以 DAW 工程采样率计。DiffScope 使用自己的速度表显示拍点和拍号。零附加延迟指样本在当前 DAW 回调交付，协议无固定延迟块，宿主侧可报告零样本附加延迟。实时期限错失时对应区段静音。宿主位置和离线标志的可用性取决于实际宿主。DAW 驱动时间线时，位置缺失的区段静音。离线标志未明确时使用实时期限。协议支持正向 1 倍速，循环和跳转以连续采样区段为单位提交。进程外 AU 需具备访问本地控制端点和共享内存的权限。

宿主走带状态和插件音频处理生命周期独立。宿主继续调用插件时，停止走带把控制权交给 DiffScope 的工程走带。宿主不提供输出回调时，工程走带停止推进，输出等待回调恢复。 [VST3 走带状态](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessContext.html) · [VST3 处理生命周期](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1Vst_1_1IAudioProcessor.html) · [AU 走带回调](https://developer.apple.com/documentation/audiotoolbox/auhosttransportstateblock) · [LV2 时间状态](https://lv2plug.in/ns/ext/time)

[iceoryx2 音频 PoC](../iceoryx2-vst-poc/README.md) · [固定版绑定示例](../iceoryx2-vst-poc/third_party/iceoryx2/examples/cxx/request_response_dynamic_data/README.md)

## 2. 现有调用接口

Audio、Synth、Core、DspxProject、ChorusKit 和 TALCS 已提供本插件所需的基础能力。服务端的集成入口如下。

| 现有接口 | Audio Bridge Server 调用方式 |
| --- | --- |
| `CoreInterface::createManagedProject(owner, fileBacked, initiallyVisible)` | 在 GUI 线程创建工程，为每个实例保留一个会话所有者。返回的窗口指针由 Core 管理。文件模式以 `fileBacked = true` 创建带空路径的文件工程，内嵌模式以 `false` 创建无文件工程。 |
| `ProjectWindowInterface::window()`、`closeManagedProject(preserveRecoveryLog)` | 使用现有窗口入口显示、隐藏和请求激活。窗口关闭按钮按 Core 的受管理窗口规则隐藏。正常卸载调用 `closeManagedProject()`，失联租期到期时调用 `closeManagedProject(true)` 保留恢复日志。 |
| `ProjectAudioContext::of(window)`、`acquireRenderSession(config)` | 在 GUI 线程取得工程 Audio 上下文，按采样率、输出声道数、块上限和实时或离线模式取得独占会话。会话沿用工程输出路由，连接期间的设置变更通过现有工程绑定生效。实时会话用于 DAW 驱动的回放和在 DAW 中输出的工程试听。空返回值表示当前配置下无法取得会话。 |
| `ProjectAudioRenderSession::prepare()`、`renderBlock(position, channels, frames)`、`release()` | GUI 线程启动离线准备并观察结果，准备成功后才开放离线读块。读块线程向调用方提供的声道缓冲输出。返回值等于请求帧数表示成功，返回 `0` 表示未输出。退出读块后在 GUI 线程释放会话。 |
| `ProjectAudioRenderSession::renderPlaybackBlockIfReady(channels, frames)` | 使用工程走带和设备输出相同的工程混音图读取试听块。检查块跨过的所有循环区段。试听停止、区段未就绪或图正在更新时返回 `0`，输出缓冲和走带位置均保持原样。 |
| `ProjectAudioContext::setExternalPlaybackState(status, position)` | 回显由 DAW 驱动的播放状态和采样位置。根据工程时间映射在采样位置与时间线刻度之间转换。 |
| `ProjectAudioContext::setExternalPlaybackController(controller)`、`setLocalPlaybackEnabled(enabled)` | 保留外部会话的输出控制权。宿主处于明确的停止状态时，将走带控制交给工程 Audio，通过现有走带动作和播放设置控制试听。 |
| `ProjectLifecycleScenario::saveAssociatedFile(context)`、`restoreProjectFile(context, path)` | 无交互保存当前关联文件，或恢复指定文件到同一工程。复用现有路径占用、校验和原位替换。 |
| `ProjectLifecycleScenario::saveProjectSnapshot(context, data)`、`restoreProjectSnapshot(context, data)` | 保存一致的 OpenDSPX 字节快照，或在当前工程恢复为无文件模式。调用端观察文件关联和恢复结果。 |

现有 Audio 会话统一处理设备、导出和外部读块之间的独占。格式兼容时可复用已打开的图。Synth 通过 Audio 的专用接口注册全工程准备器及区段就绪判定。试听和 DAW 驱动的实时回放共用同一实时会话。离线会话通过已注册的准备器等待并复查就绪状态。准备成功后由 Audio 持有文档读锁，直至会话释放。TALCS 在合成序列离开 `Notify` 时归还该序列自己的缓冲暂停。服务端以会话准备结果、读块返回值和会话生命周期为准。

[Core 窗口创建](src/plugins/coreplugin/core/CoreInterface.h) · [工程窗口接口](src/plugins/coreplugin/windows/ProjectWindowInterface.h) · [Audio 会话](src/plugins/audio/audiocontext/ProjectAudioContext.h) · [工程生命周期](src/libs/application/dspxproject/src/scenarios/ProjectLifecycleScenario.h)

## 3. 控制连接

### 3.1 端点和线程

控制信道使用本地可靠字节流，服务端使用 `QLocalServer`。客户端和服务端使用协议头文件中的固定名称 `diffscope-bridge.sock`，按下表组成完整地址。该地址在进程重启后沿用，协议版本在 `Attach` 中协商。同一地址接受多个客户端连接。

| 平台 | 控制端点 | 名称来源 |
| --- | --- | --- |
| Windows | `\\.\pipe\diffscope-bridge.sock` | 添加本机命名管道前缀 `\\.\pipe\`。 [命名管道名称](https://learn.microsoft.com/en-us/windows/win32/ipc/pipe-names) |
| Linux | `${XDG_RUNTIME_DIR}/diffscope-bridge.sock` | 使用当前用户的 XDG 运行时目录。 [XDG 运行时目录](https://specifications.freedesktop.org/basedir/) |
| macOS | `<userRuntimeDir>/diffscope-bridge.sock` | 使用 `confstr(_CS_DARWIN_USER_TEMP_DIR)` 返回的当前用户临时目录。 [用户临时目录](https://developer.apple.com/library/archive/documentation/Security/Conceptual/SecureCodingGuide/Articles/RaceConditions.html) |

Windows 通过 `QLocalServer::UserAccessOption` 将管道访问限制在当前用户。Unix 使用受限运行时目录并检查对端 UID。Unix 监听前使用控制套接字路径加 `.lock` 的 `QLockFile` 取得监听权，以便清理上次异常退出遗留的套接字。随机音频服务名随所属连接的配置回复公开，iceoryx2 端点沿用对应的用户访问限制。 [Qt 本地端点监听规则](https://doc.qt.io/qt-6.10/qlocalserver.html#listen)

本地字节流提供有序控制消息、连接断开通知及变长状态包的分段收发。控制 I/O 线程负责连接、帧解析、状态包收发、心跳和断线检测。窗口、文档及 Audio 会话的控制操作提交到 GUI 线程。GUI 线程持有工程会话和其窗口所有者，并观察准备结果、窗口可见性及文件关联。专用音频 IPC 线程管理共享内存端口及借用区。各工程的读块工作线程只在关联会话有效时拉取音频，图读取与其他工程的 IPC 分开。

### 3.2 会话生命周期

客户端为每个实例提供随机的 128 位 `instanceId`，此标识在该实例生命周期内保持稳定。首次 `Attach` 为该实例创建工程窗口。重新连接在租期内绑定原工程，恢复最新编辑器可见意图并重新配置音频端口。实例仍有活动连接或正在释放上一次连接的音频会话时，`Attach` 返回 `Busy`。连接协议兼容时，即使音频 ABI 不匹配，也可以操作工程并收取音频错误状态。

`Detach` 结束会话。控制连接断开时，服务端停止接收该连接的请求，立即隐藏窗口并开始计算剩余租期，同时停止音频端口接单并取消离线准备。若有在途读块，GUI 线程收到读块完成通知后释放渲染会话和端口。尚未提交的控制操作取消，已经提交的工程操作以其结果为准。心跳间隔初值为 1 s，租期初值为最近一次有效流量后的 5 s。合法状态帧的实际接收进度计入活跃度。租期到期且音频资源释放后关闭工程并释放文件归属。会话所有者独立于套接字生命周期，使短暂断线期间工程保持活跃。

应用退出时先停止监听，撤销活动连接及音频端口，取消准备并等待在途读块归还资源，随后关闭相关工程窗口。断线时可见状态由客户端根据连接生命周期更新。服务端在连接建立后发送当前工程和音频状态。

### 3.3 控制帧

控制帧头为 16 字节，小端。字段依次为 `bodyBytes:u32`、`opcode:u16`、`flags:u16`、`requestId:u64`。`bodyBytes` 是紧随帧头的二进制正文长度。请求的 `flags` 为零，成功回复为 `Reply`，错误回复为 `Reply | Error`，服务端事件为 `Event`。其余位为零。回复回显请求 ID 和操作码。每条连接上的请求 ID 从非零值递增，事件的请求 ID 为零。正文由操作码和帧类型共同决定，错误回复共用一种正文布局。具体定义见附录 A。

控制 I/O 线程按正文类型校验长度。空正文和定长正文校验精确大小，变长文本按布局中的长度字段校验。携带工程状态包的正文按协商的状态包上限校验，`Attach` 在协商完成前使用服务端的 128 MiB 上限，并在读到固定字段后纳入客户端声明的上限。按实际接收进度读入，完整校验后才派发。每实例同时执行至多一个持久化操作。

| 操作码 | 请求及服务端结果 |
| --- | --- |
| `1 Attach` | 请求包括 `instanceId`、`protocolVersion`、`audioAbi`、`editorVisible`、状态大小上限、初始存储模式及可选状态包。控制版本不兼容时返回 `Incompatible`。成功绑定时回复 `resumed`、协商后的大小上限和工程状态。新实例的初始载入失败时保留空工程，在成功绑定的状态字段中报告加载错误。重新绑定时保留工程，新的恢复意图使用 `RestoreState`。 |
| `2 Ping`、`3 Detach` | 心跳由控制 I/O 线程直接回复。分离请求停止该会话，并关闭对应工程。 |
| `4 ConfigureAudio` | 请求包括 `sampleRate`、`channels`、`maxFrames`。Bridge 客户端固定请求 32 个输出声道。成功时回复音频服务名、唤醒服务名、有效块上限和 `generation`。重配时退役旧端口。 |
| `5 ReleaseAudio` | 退役当前端口并释放渲染权，保留工程。 |
| `6 Transport` | 回报宿主 `Playing`、`Stopped`、`Unknown`，并附可用的工程采样位置。实时模式下，宿主停止走带时允许 `Play`、`Pause`、`Stop`、`Seek` 控制输出到 DAW 的试听。DAW 驱动实时回放或离线批次进行时，工程试听命令返回 `TransportOwnedByHost`。`Seek` 使用协商的采样率及有效采样位置。 |
| `7 BeginOffline`、`8 EndOffline` | 启动或结束离线批次。准备成功后 `BeginOffline` 返回可读块的代际。`EndOffline` 也可取消正在等待的开始请求。恢复实时配置后发布新的代际。每个会话至多一个离线批次。 |
| `9 SetEditorVisible`、`10 RevealProject` | 编辑器打开和显式唤起均会恢复最小化的工程窗口、显示窗口并请求激活。编辑器关闭时隐藏窗口。 |
| `11 SaveState`、`12 RestoreState` | 无交互保存并取得完整状态包，或载入完整状态包。回复以操作完成时的结果为准。 |
| `13 Status` | 服务端事件。报告工程加载、音频通道就绪和离线准备状态、窗口实际可见性以及最近错误。文件工程报告已提交的 `externalPath`，无文件关联时为空。归纳期限错失或源不可用等音频诊断，避免逐块发送控制消息。 |
| `15 ProjectAction` | 执行插件编辑器发起的工程操作。请求包含操作枚举，成功回复包含结果工程的完整状态包。 |

工程恢复、音频重配和离线模式切换按会话顺序提交。`Ping`、断线回收和取消离线准备可在其他请求等待期间处理。会话所有者只投递当前连接的回复。状态变化既可出现在命令回复中，也可由 `Status` 主动通知，例如窗口关闭按钮隐藏窗口或用户另存为。

## 4. 音频信道

### 4.1 服务和块格式

服务端每次建立一组音频端口时生成一个随机的 128 位标识，编码为 32 个小写十六进制字符。该标识作为 `<token>`，同时用于两个 iceoryx2 `ServiceType::Ipc` 服务名。

| 用途 | 服务名 |
| --- | --- |
| 音频 Request/Response | `DiffScope/Bridge/Audio/<token>` |
| 唤醒 Event | `DiffScope/Bridge/Wake/<token>` |

`ConfigureAudio` 在端口创建成功后返回两个完整名称，客户端按返回值打开服务。音频重配或断线后重建端口使用新标识，旧端口退役。每个音频服务一个客户端、一个服务端。协商有限正数采样率、输出声道数及有效最大块长。块上限初值为 4096。端口按 `maxFrames × channels` 个样本配置每块容量。双方固定 ABI 名称、字段偏移、小端字节序和对齐。

`channels` 可为 `1`、`2` 或 `4` 至 `32` 之间的偶数。其他取值返回 `InvalidRequest`。声道上限与 Audio 渲染会话一致，受 TALCS 的 32 位声道静音掩码限制。客户端按总线声明顺序及总线内的左右声道顺序映射固定的 32 个声道。宿主停用某条总线时，该总线仍占用原有的两个协议声道，其他总线维持原有映射。LV2 按声明的音频输出端口顺序映射。需要变更采样率或块上限时重新调用 `ConfigureAudio`。

| 类型 | 布局和语义 |
| --- | --- |
| `AudioRequestV1` | 24 字节、8 字节对齐。依次为 `sequence:u64`、`projectFrame:i64`、`generation:u32`、`frames:u32`。请求序号在实例内递增。DAW 驱动时，位置以协商采样率计。实时试听时 `projectFrame` 置零，使用工程走带的位置。 |
| `AudioReplyHeaderV1` | 32 字节、8 字节对齐。前 24 字节回显请求，随后为 `status:u32`、`reserved:u32`。状态为 `0 Ok`、`1 NotReady`、`2 InvalidRequest`、`3 ReadFailed`、`4 Cancelled`。 |
| 回复载荷 | 成功时为 `Slice<float>`，含 `frames × channels` 个样本，每个声道连续。错误回复载荷为空。 |

每个会话使用单一 `generation`。音频配置替换、工程内容整体替换及时间线控制权交接使旧代际失效。发布新代际前拒绝旧请求，准备期间返回 `NotReady`。服务端检查代际、帧数及实际渲染位置的范围。DAW 驱动时，跳转的目标由下个区段的请求位置表达，负位置及跨零区段由客户端按协议在请求前处理。零帧无需音频请求。

### 4.2 借用区与实时期限

1. 控制阶段创建端口，配置固定容量和有界队列，采用 PoC 的 `DiscardData`。每个客户端最多一个待消费请求，回复队列容量为 1。过期请求释放对应的 `PendingResponse`。服务端同一工程最多有一个读块在执行。
2. 服务端使用专用音频 IPC 线程轮询各活动服务。空闲时可通过 iceoryx2 `Event` 和 `WaitSet` 等待唤醒。通知表示队列可能有工作，进入等待前复查队列。每轮对各服务处理有界数量的请求和结果，避免一个忙碌实例占用整个 IPC 线程。
3. IPC 线程通过 `ActiveRequest::loan_slice_uninit()` 借出回复切片，用 `response_user_header` 写块头。读块工作线程取得可写声道视图，按播放模式调用 `renderBlock()` 或 `renderPlaybackBlockIfReady()`。双声道和多声道直接写入共享区，单声道经会话的预分配立体声工作缓冲下混。IPC 线程在读块返回请求帧数后发送成功回复。
4. 端口及 loan 始终由 IPC 线程管理。IPC 与读块线程通过预分配槽交接缓冲视图及完成结果。停止接单或取消后，在途工作可以结束。实际持有视图的线程归还后，IPC 线程才能回收 loan。发送前复查当前代际及请求连接，丢弃旧代际和已取消的结果。
5. DAW 实时请求的本地期限由当前回调的样本数和采样率计算。客户端收到有效回复后立即将共享区中的 PCM 按声道顺序复制到宿主缓冲。服务端按序号和代际关联结果，无需跨进程比较时钟。端口容量不足时放弃当前实时块，离线请求报告错误。

Audio 和 TALCS 的图读取可能包含锁、读盘及效果处理。TALCS 在读块需要更多声道时扩展内部工作缓冲。由工程读块线程承担这些操作，避免阻塞其他会话的 IPC。服务端以实际工程的回调期限和超期数量衡量同回调交付。PoC 的内存 WAV 时延数据只适用于其测试条件。

[固定版动态回复示例](../iceoryx2-vst-poc/third_party/iceoryx2/examples/cxx/request_response_dynamic_data/README.md) · [WaitSet 接口](../iceoryx2-vst-poc/third_party/iceoryx2/iceoryx2-cxx/include/iox2/waitset.hpp)

### 4.3 播放控制与输出

- **DAW 驱动的实时回放。** 使用 `Realtime` 会话的 `renderBlock()` 读取宿主回调指定的工程采样区段。未完成的合成片段静音，其他轨道继续输出。宿主开始走带时接管工程位置并中止试听。回放状态和位置在 GUI 线程通过 `setExternalPlaybackState()` 回显。图配置期间按 `NotReady` 处理。
- **停止走带时的试听。** 音频端口和 `Realtime` 会话继续有效。播放、暂停、停止、定位、循环和播放头偏好由 Audio 的工程走带处理，和设备播放共用工程输出混音器。读块返回 `0` 时回复 `NotReady`，DAW 回调输出静音，工程走带位置保留。区段就绪后，从当前走带位置继续渲染。宿主走带状态未知时，等待明确的 `Stopped` 再允许试听。
- **实时控制权交接。** 客户端按回调中有效的宿主走带信息报告状态，缺少走带信息时报告 `Unknown`。走带状态变化时发送 `Transport`，同时停止提交旧代际数据。收到 `Status` 发布的新代际后按该模式请求新块。切换复用已配置的 `Realtime` 会话。开始和停止走带都无需重开图或切换音频设备。停止音频处理后，端口由 `ReleaseAudio` 或断线等生命周期事件回收。
- **离线接管。** `BeginOffline` 使原代际停止接单，待在途读块结束后交还旧会话，以 `Offline` 取得会话并调用 `prepare()`。GUI 线程异步等待准备结果。成功后发布离线代际，后续读块使用已由 Audio 持有的文档读锁，按 DAW 指定的位置渲染。准备失败、源不可用及取消分别映射为控制错误。
- **离线结束。** `EndOffline`、端口释放、断线或工程恢复取消离线准备并停止接单，待在途读块结束后调用 `release()`。端口继续工作时重新配置实时会话，按宿主最新走带状态确定时间线控制权。离线或实时模式的变更通过新代际生效。

工程现有的播放、暂停、停止和切换动作由 Audio 处理。`Transport` 的本地指令调用同一工程走带入口，定位通过工程时间线传递。服务端在 DAW 驱动回放时通过 `setExternalPlaybackState()` 回显位置。 [现有播放动作](src/plugins/audio/qml/actions/PlaybackActions.qml) · [Audio 播放状态入口](src/plugins/audio/audiocontext/ProjectAudioContext.h)

## 5. 工程持久化

### 5.1 模式及状态包

宿主加载插件实例时，Bridge 立即连接。实例没有宿主持久化状态时，首次 `Attach` 使用 `HostState`，DiffScope 创建无文件的空工程。实例有持久化状态时，服务端按成功载入的状态包确定模式。短暂断线重连沿用同一工程当前的模式。

| 模式 | 服务端动作 |
| --- | --- |
| `ExternalFile` | `SaveState` 调用 `saveAssociatedFile()`，成功后只返回当前绝对路径。`RestoreState` 调用 `restoreProjectFile()`，成功后在原窗口关联该文件。用户在窗口内可以按 Core 的既有流程保存或另存为。 |
| `HostState` | `SaveState` 调用 `saveProjectSnapshot()`，`RestoreState` 调用 `restoreProjectSnapshot()`。快照包含 OpenDSPX 数据、工作区及已嵌入媒体，外部素材继续按项目引用保存。受管理的无文件工程采用现有的文件动作限制，工程内容由宿主请求保存及恢复。 |

状态包头为 4 字节，依次为小端 `version:u16`、`kind:u8`、`reserved:u8`。v1 中 `version=1`、`reserved=0`。`kind=1` 后跟 UTF-8 绝对路径，`kind=2` 后跟 OpenDSPX 字节。状态包直接作为 `RestoreState` 请求、`SaveState` 成功回复和 `ProjectAction` 成功回复的全部正文，也可附在 `Attach` 请求的固定字段后。状态包长度由对应正文的剩余字节数确定。首次连接时可以声明文件模式并从未关联路径的空工程开始。需要执行文件保存或恢复时，路径为空返回 `NeedsProjectPath`。状态包超出协商上限返回 `StateTooLarge`。

恢复先校验包头、模式及大小，再停止旧音频代际，退出在途读块并释放 Audio 会话，随后在 GUI 线程调用现有工程恢复入口。成功时切换持久化模式和文件关联，按最新宿主状态恢复音频。提交前失败时保留原工程及文件关联，返回错误后可为原工程重新建立音频会话。保存按既有入口捕获一致数据，操作失败时回复错误。宿主可继续使用上次成功取得的完整状态包。

工程恢复期间，服务端观察 `aboutToReload()` 以停止接单，使用 `reloaded()` 和 Audio 的 `renderSessionChanged()` 更新状态。文档写入结束后按当前宿主状态重新申请会话，取得新代际后再开放读块。

启用文件日志时，工程初始化时原子提交恢复快照，后续文档事务持续追加到同一日志。保存及替换活动工程期间保留已写入的恢复记录。恢复现有日志时以只读方式打开源记录，并为恢复出的工程建立独立日志。

### 5.2 文件归属和状态通知

文件归属由现有 `ProjectLifecycleScenario` 管理。普通工程和 Bridge 工程共用同一规则。已关联路径只能属于一个活跃工程，隐藏窗口和重连租期内的窗口仍持有归属。「文件 → 打开」在新窗口打开工程，或在该文件已打开时激活对应的窗口。Bridge 恢复另一工程占用的文件时返回 `PathInUse`，保留实例当前内容和关联。首次恢复失败时保留未关联的空工程及错误信息，宿主可重试。

服务端对 `fileLockerChanged()` 重绑路径观察，并通过 locker 的 `pathChanged()` 和工程恢复结果更新 `externalPath`。另存为成功后才发布新文件关联。保存副本仅在目标写入，当前关联保持原值。DAW 需要再次保存其工程才会把新路径写入 DAW 工程文件。关联查找、别名比较、覆盖确认、原子写入及外部素材相对路径计算均由现有流程负责。

| 调用结果 | 控制错误 |
| --- | --- |
| 控制请求正文与协议布局不符，或参数取值无效 | `InvalidRequest` |
| 控制版本不兼容，或音频配置使用不兼容的音频 ABI | `Incompatible` |
| 已被另一工程关联 | `PathInUse` |
| 工程操作冲突或独占图暂时不可用 | `Busy` |
| 宿主持有工程渲染权时请求本地控制 | `TransportOwnedByHost` |
| 无法建立音频端口或打开会话所需的音频图 | `AudioUnavailable` |
| 文件外部变更、校验或完整性警告需要人工确认 | `NeedsConfirmation` |
| 路径缺失、状态包超限或状态包结构无效 | `NeedsProjectPath`、`StateTooLarge` 或 `InvalidState` |
| 读取、解码、载入或写入失败 | `ReadFailed`、`LoadFailed` 或 `WriteFailed` |
| 离线合成准备失败、源不可用或操作取消 | `PreparationFailed`、`SourceUnavailable` 或 `Cancelled` |

### 5.3 插件编辑器的工程操作

Bridge 插件编辑器通过 `Status` 显示实际存储模式和关联路径。用户发起工程操作时，Bridge 发送 `ProjectAction` 请求。需要路径时，DiffScope 在该实例的工程窗口弹出文件对话框。

| 操作 | 工程行为 |
| --- | --- |
| `LinkFile` | 从选定的工程文件读取数据，在同一窗口替换工程并关联文件。检查文件归属和 UTF-8 绝对路径长度。成功回复包含外部文件状态包。 |
| `ImportToHost` | 以只读方式载入选定的工程文件，在同一窗口替换工程并采用宿主内嵌模式。该操作不取得源路径的归属。以源文件所在目录解析相对素材。提交前生成快照并验证快照大小。成功回复包含宿主状态包。 |
| `SaveToFile` | 通过 `saveProjectToFile()` 保存尚未关联文件的工程。复用路径占用检查、覆盖确认和原子写入，写入成功后关联文件。成功回复包含外部文件状态包。 |
| `StoreInHost` | 通过 `detachProjectFile()` 捕获已关联工程的快照并解除关联。保留当前编辑及源目录，磁盘上的外部文件保留原内容。成功回复包含宿主状态包。 |

关联和导入复用工程校验，需要放弃当前未保存修改时要求确认。工程替换沿用音频会话的重载流程。保存到外部文件和解除关联保留当前工程内容及撤销历史。工程操作与宿主持久化在同一实例内串行执行。断线或卸载时取消文件选择和待决确认。取消或提交前失败时保留当前工程和关联。Bridge 收到成功回复后更新缓存并通知宿主工程状态变化。DAW 随后保存工程时，通过 `SaveState` 取得当前状态包。

内嵌工程的普通「另存为」禁用。外部文件模式的普通「另存为」更新当前关联路径。两种模式均提供「保存副本」，副本使用独立的目标路径。

## 6. 插件集成验收条件

- **插件生命周期。** Audio Bridge Server 在插件初始化完成后接受控制连接，在应用退出时关闭监听、工程会话和 IPC 工作线程。
- **控制连接与工程会话。** 多个实例各自持有工程，短暂断线可恢复原工程，卸载或持续失联后关闭窗口并回收文件归属。窗口显隐和工程关联变更通过状态事件发布。
- **音频端口与渲染调度。** 实时和离线渲染均采用工程的输出路由。实时块在当前 DAW 回调交付或按期限静音。离线首块在准备成功后开放。DAW 停止走带且继续调用插件时，DiffScope 可使用原有工程走带试听，未就绪区段保持走带位置，就绪后从该位置继续向 DAW 输出。
- **持久化协议。** 外部模式保存工程路径，内嵌模式返回快照。外部路径冲突和恢复失败得到明确结果。另存为成功后向宿主发布新关联。存储模式可双向转换，取消或失败后保留原模式及文件关联。

## 附录 A. IPC 数据布局

以下布局使用 C++ POD 数据类型。柔性数组表示定长字段后紧接的变长字节。`data[]` 一类声明使用编译器的柔性数组扩展，标准 C++20 实现可将定长字段和尾部字节分别处理。字段注释中的数字为从各自结构起点计的字节偏移。多字节整数和 IEEE 754 浮点数采用小端字节序。线上长度按字段偏移和实际字节数计算，接收时逐字段读取。

### A.1 控制帧和状态包

控制帧从偏移 16 开始存放 `bodyBytes` 个正文字节。`flags` 保存 `ControlFrameFlag` 位掩码。状态包从偏移 4 开始按 §5.1 存放 UTF-8 路径或 OpenDSPX 快照。UTF-8 字段使用字节计数。文字紧密排列，省略终止符和字段之间的填充。

```cpp
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

enum class ControlOpcode : std::uint16_t {
    Attach = 1,
    Ping = 2,
    Detach = 3,
    ConfigureAudio = 4,
    ReleaseAudio = 5,
    Transport = 6,
    BeginOffline = 7,
    EndOffline = 8,
    SetEditorVisible = 9,
    RevealProject = 10,
    SaveState = 11,
    RestoreState = 12,
    Status = 13,
    ProjectAction = 15,
};

enum class ControlFrameFlag : std::uint16_t {
    Reply = 0x0001,
    Error = 0x0002,
    Event = 0x0004,
};

struct ControlFrameV1 {
    std::uint32_t bodyBytes; // 0
    ControlOpcode opcode;    // 4
    std::uint16_t flags;     // 6
    std::uint64_t requestId; // 8
    std::uint8_t body[];     // 16
};

enum class BridgeStateKind : std::uint8_t {
    ExternalFile = 1,
    HostState = 2,
};

struct BridgeStatePacketV1 {
    std::uint16_t version;  // 0
    BridgeStateKind kind;   // 2
    std::uint8_t reserved;  // 3
    std::uint8_t payload[]; // 4
};

static_assert(offsetof(ControlFrameV1, body) == 16);
static_assert(offsetof(BridgeStatePacketV1, payload) == 4);
```

### A.2 控制正文

| 操作码 | 请求正文 | 成功回复或事件正文 |
| --- | --- | --- |
| `Attach` | `AttachRequestV1` | `AttachReplyV1` |
| `Ping`、`Detach`、`ReleaseAudio`、`RevealProject` | 空 | 空 |
| `ConfigureAudio` | `ConfigureAudioRequestV1` | `ConfigureAudioReplyV1` |
| `Transport` | `TransportRequestV1` | 空 |
| `BeginOffline`、`EndOffline` | 空 | `AudioGenerationReplyV1` |
| `SetEditorVisible` | `SetEditorVisibleRequestV1` | 空 |
| `SaveState` | 空 | `SaveStateReplyV1` |
| `RestoreState` | `RestoreStateRequestV1` | 空 |
| `Status` | 仅服务端事件 | `StatusEventV1` |
| `ProjectAction` | `ProjectActionRequestV1` | `ProjectActionReplyV1` |

空正文的 `bodyBytes` 为 `0`。错误回复统一使用 `ControlErrorReplyV1`，无论操作码的成功正文采用何种布局。布尔字段使用 `0` 或 `1`。发送端将保留字段置零。`sampleRate` 为有限正数，采用 IEEE 754 binary64。

```cpp
enum class ControlErrorCode : std::uint16_t {
    None = 0,
    InvalidRequest = 1,
    Incompatible = 2,
    Busy = 3,
    InvalidState = 4,
    TransportOwnedByHost = 5,
    NeedsProjectPath = 6,
    StateTooLarge = 7,
    PathInUse = 8,
    NeedsConfirmation = 9,
    ReadFailed = 10,
    LoadFailed = 11,
    WriteFailed = 12,
    PreparationFailed = 13,
    SourceUnavailable = 14,
    Cancelled = 15,
    AudioUnavailable = 16,
};

enum class HostTransportState : std::uint8_t {
    Unknown = 0,
    Stopped = 1,
    Playing = 2,
};

enum class LocalTransportCommand : std::uint8_t {
    None = 0,
    Play = 1,
    Pause = 2,
    Stop = 3,
    Seek = 4,
};

enum class ProjectLoadState : std::uint8_t {
    Ready = 0,
    Loading = 1,
    LoadFailed = 2,
};

enum class AudioPortState : std::uint8_t {
    Unconfigured = 0,
    Idle = 1,
    RealtimeReady = 2,
    OfflinePreparing = 3,
    OfflineReady = 4,
};

struct AttachRequestV1 {
    std::uint8_t instanceId[16]; // 0, opaque bytes
    std::uint32_t maxStateBytes; // 16, including the state packet header
    std::uint16_t protocolVersion; // 20
    std::uint16_t audioAbi; // 22
    BridgeStateKind initialKind; // 24
    std::uint8_t editorVisible; // 25
    std::uint16_t reserved; // 26
    std::uint8_t statePacket[]; // 28, optional BridgeStatePacketV1
};

struct AttachReplyV1 {
    std::uint32_t maxStateBytes; // 0, negotiated state packet limit
    std::uint8_t resumed; // 4
    std::uint8_t reserved[3]; // 5
    std::uint8_t status[]; // 8, one complete StatusEventV1 body
};

inline constexpr std::uint8_t BridgeMaxAudioChannels = 32;

struct ConfigureAudioRequestV1 {
    double sampleRate; // 0, Hz
    std::uint32_t maxFrames; // 8
    std::uint8_t channels; // 12, total enabled output channel count
    std::uint8_t reserved[3]; // 13
};

struct ConfigureAudioReplyV1 {
    std::uint32_t generation; // 0
    std::uint32_t maxFrames; // 4, effective block limit
    std::uint16_t audioNameBytes; // 8
    std::uint16_t wakeNameBytes; // 10
    std::uint8_t names[]; // 12, audio service name followed by wake service name
};

struct TransportRequestV1 {
    std::int64_t projectFrame; // 0
    HostTransportState hostState; // 8
    LocalTransportCommand command; // 9
    std::uint8_t hasPosition; // 10
    std::uint8_t reserved[5]; // 11
};

struct AudioGenerationReplyV1 {
    std::uint32_t generation; // 0
};

struct SetEditorVisibleRequestV1 {
    std::uint8_t visible; // 0
};

using SaveStateReplyV1 = BridgeStatePacketV1;
using RestoreStateRequestV1 = BridgeStatePacketV1;

enum class ProjectAction : std::uint8_t {
    LinkFile = 1,
    ImportToHost = 2,
    SaveToFile = 3,
    StoreInHost = 4,
};

struct ProjectActionRequestV1 {
    ProjectAction action; // 0
};

using ProjectActionReplyV1 = BridgeStatePacketV1;

struct StatusEventV1 {
    std::uint32_t generation; // 0
    ControlErrorCode lastError; // 4
    ProjectLoadState projectState; // 6
    AudioPortState audioState; // 7
    BridgeStateKind storageKind; // 8
    std::uint8_t windowVisible; // 9
    std::uint16_t externalPathBytes; // 10
    std::uint16_t detailBytes; // 12
    std::uint16_t reserved; // 14
    std::uint8_t text[]; // 16, external path followed by diagnostic detail
};

struct ControlErrorReplyV1 {
    ControlErrorCode code; // 0, nonzero
    std::uint16_t detailBytes; // 2
    std::uint8_t detail[]; // 4, diagnostic detail
};

static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
static_assert(offsetof(AttachRequestV1, statePacket) == 28);
static_assert(offsetof(AttachReplyV1, status) == 8);
static_assert(sizeof(ConfigureAudioRequestV1) == 16);
static_assert(offsetof(ConfigureAudioReplyV1, names) == 12);
static_assert(sizeof(TransportRequestV1) == 16);
static_assert(sizeof(AudioGenerationReplyV1) == 4);
static_assert(sizeof(SetEditorVisibleRequestV1) == 1);
static_assert(sizeof(ProjectActionRequestV1) == 1);
static_assert(offsetof(StatusEventV1, text) == 16);
static_assert(offsetof(ControlErrorReplyV1, detail) == 4);
```

- **握手。** v1 的 `protocolVersion` 和 `audioAbi` 均为 `1`，两者分别校验。`AttachRequestV1` 的正文长度为 `28 + 状态包字节数`，尾部长度为零表示没有状态包。`initialKind` 决定新建空工程的模式，成功恢复后采用状态包中的模式。`maxStateBytes` 计入状态包固定头，协商值取服务端与客户端上限的较小值。`AttachReplyV1` 的正文长度为 `8 + StatusEventV1 正文字节数`，内嵌状态按 `StatusEventV1` 相同规则解析。初始工程载入失败或音频 ABI 不兼容时，通过内嵌状态报告错误。
- **音频配置和控制。** `ConfigureAudioRequestV1` 的 `maxFrames` 必须大于零，`channels` 须符合 §4.1。`ConfigureAudioReplyV1` 的正文长度为 `12 + audioNameBytes + wakeNameBytes`，两个服务名均非空。`TransportRequestV1` 的 `command` 为 `None` 时仅报告宿主状态。`hasPosition` 为 `0` 时 `projectFrame` 置零。`Seek` 要求 `hasPosition` 为 `1` 且采样率已协商。`AudioGenerationReplyV1` 发布当前可用于音频请求的代际，`0` 表示端口已失效或没有已发布的代际。
- **状态和错误。** `StatusEventV1` 的正文长度为 `16 + externalPathBytes + detailBytes`。`Idle` 表示端口已配好，当前没有可读的渲染会话。没有已发布的代际时 `generation` 为 `0`。没有已关联路径时 `externalPathBytes` 为 `0`。`ControlErrorReplyV1` 的正文长度为 `4 + detailBytes`。`lastError` 为 `None` 表示当前状态无错误。错误回复的 `code` 必须非零。诊断文本可为空。
- **状态包长度。** `SaveStateReplyV1`、`RestoreStateRequestV1` 和 `ProjectActionReplyV1` 的 `bodyBytes` 等于完整状态包长度，至少为 4 字节。带状态包的 `AttachRequestV1` 按定长字段后的剩余长度校验状态包。各变长字段按正文剩余字节数依次校验，最后一个字段结束后正文恰好耗尽。

### A.3 音频请求和回复

请求载荷为 `AudioRequestV1`。回复的 iceoryx2 用户头为 `AudioReplyHeaderV1`，音频载荷是独立的 `AudioSampleV1` 切片。成功时切片含 `request.frames × channels` 个 IEEE 754 binary32 样本，按声道连续排列。声道 `channel` 内第 `frame` 帧的样本索引为 `channel * request.frames + frame`。错误回复的切片长度为零。

```cpp
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

struct alignas(8) AudioRequestV1 {
    std::uint64_t sequence;    // 0
    std::int64_t projectFrame; // 8, zero for realtime preview
    std::uint32_t generation;  // 16
    std::uint32_t frames;      // 20
};

enum class AudioReplyStatus : std::uint32_t {
    Ok = 0,
    NotReady = 1,
    InvalidRequest = 2,
    ReadFailed = 3,
    Cancelled = 4,
};

struct alignas(8) AudioReplyHeaderV1 {
    AudioRequestV1 request;  // 0, echoed request
    AudioReplyStatus status; // 24
    std::uint32_t reserved;  // 28
};

using AudioSampleV1 = float;

static_assert(std::is_trivial_v<AudioRequestV1> && std::is_standard_layout_v<AudioRequestV1>);
static_assert(sizeof(AudioRequestV1) == 24 && alignof(AudioRequestV1) == 8);
static_assert(std::is_trivial_v<AudioReplyHeaderV1> && std::is_standard_layout_v<AudioReplyHeaderV1>);
static_assert(sizeof(AudioReplyHeaderV1) == 32 && alignof(AudioReplyHeaderV1) == 8);
static_assert(offsetof(AudioReplyHeaderV1, status) == 24);
static_assert(sizeof(AudioSampleV1) == 4 && std::numeric_limits<AudioSampleV1>::is_iec559);
```
