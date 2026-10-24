using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Windows.Input;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.UI.Controls;
using GsmTools.Desktop.Workflows;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.ViewModels;

public class MainWindowViewModel : ViewModelBase
{
    private readonly ISamsungCscWorkflow _cscWorkflow;
    private readonly IDeviceTelemetryWorkflow _telemetryWorkflow;
    private readonly ISimLockWorkflow _simLockWorkflow;
    private readonly IBoostSprintWorkflow _boostWorkflow;
    private readonly IRadioControlWorkflow _radioWorkflow;
    private readonly IInteractiveAtWorkflow _interactiveWorkflow;

    // --- Device Telemetry Properties ---
    private string _model = "SM-S918U";
    private string _serial = "RFCX90GR21X";
    private string _imei = "355752920542633";
    private string _currentCsc = "ATT";
    private string _productCode = "SM-S918UZKVATT";
    private string _buildNumber = "S918USQS6DYG1";
    private string _androidVersion = "15";
    private string _securityPatch = "2025-07-01";
    private string _usbConfig = "mtp,adb";
    private string _basebandVersion = "S918USQS6DYG1";
    private string _imsi = "310410012345678";
    private string _hardwareLock = "NONE";
    private string _rawDevConInfo = "+DEVCONINFO: MN(SM-S918U);BASE(S918U);VER(S918USQS6DYG1);MNC(410);MCC(310);PRD(ATT);LOCK(NONE);";

    // --- Status Badges ---
    private string _selinuxStatus = "Enforcing";
    private string _rsuAppStatus = "Clean";
    private string _tsdaemonStatus = "Clean";
    private string _port2600Status = "Connected uid=1000";
    private string _retailAgentStatus = "5.59.6-main";
    private string _atChannelStatus = "/dev/umts_ipc0";
    private string _simCardStatus = "Ready";
    private string _networkLockStatus = "UNLOCKED (0)";

    // --- CSC Workflow Properties ---
    private string _selectedCsc = "XAA";
    private int _selectedMethodIndex = 1;
    private string _artifactsDirectory = @"C:\Users\pc5\Documents\GitHub\gsmtools\2";
    private bool _useEncryptedSession = false;

    // --- SIM & Network Locks Properties ---
    private string _cpinState = "READY";
    private string _networkLockPn = "UNLOCKED (0)";
    private string _networkSubsetLockPu = "UNLOCKED (0)";
    private string _serviceProviderLockPp = "UNLOCKED (0)";
    private string _corporateLockPc = "UNLOCKED (0)";
    private string _simPinLockSc = "UNLOCKED (0)";
    private string _simLockSummary = "All Network Personalization Locks Clear";
    private bool _isSimFullyUnlocked = true;

    // --- Sprint / Boost & Relock Properties ---
    private string _mccMncSearchQuery = "";
    private MccMncEntry? _selectedCarrier;
    private string _customMcc = "421";
    private string _customMnc = "01";
    private string _calculatedBcdPayload = "421F0100";

    // --- Security & Radio Properties ---
    private string _securitySeed = "Not Queried";

    // --- Interactive AT Terminal ---
    private string _terminalInput = "AT+DEVCONINFO";

    // --- Common Orchestration Properties ---
    private bool _isBusy = false;
    private string _statusText = "Ready - GsmTools Multi-Modal System v3.0 (Sprint/Boost, Relock, Telemetry, CSC, AT Terminal).";
    private double _progressValue = 0;

    public ObservableCollection<string> AvailableCscs { get; } = new()
    {
        "XAA - USA Unbranded / Open",
        "USC - US Cellular",
        "TMB - T-Mobile USA",
        "ATT - AT&T USA",
        "VZW - Verizon USA",
        "SPR - Sprint USA",
        "BST - Boost Mobile USA",
        "GCF - Global Certification",
        "FKR - FKR USA",
        "XAG - USA Open Variant",
        "XAS - Sprint / T-Mobile USA",
        "XAB - USA Open",
        "EUX - European Open",
        "KOO - Korea Open",
        "THL - Thailand (Auto Call Recording)",
        "MID - Middle East"
    };

    public ObservableCollection<MccMncEntry> AvailableCarriers { get; } = new();
    public ObservableCollection<ConsoleLogItem> LogItems { get; } = new();

    // Getters and Setters
    public string Model { get => _model; set => SetProperty(ref _model, value); }
    public string Serial { get => _serial; set => SetProperty(ref _serial, value); }
    public string Imei { get => _imei; set => SetProperty(ref _imei, value); }
    public string CurrentCsc { get => _currentCsc; set => SetProperty(ref _currentCsc, value); }
    public string ProductCode { get => _productCode; set => SetProperty(ref _productCode, value); }
    public string BuildNumber { get => _buildNumber; set => SetProperty(ref _buildNumber, value); }
    public string AndroidVersion { get => _androidVersion; set => SetProperty(ref _androidVersion, value); }
    public string SecurityPatch { get => _securityPatch; set => SetProperty(ref _securityPatch, value); }
    public string UsbConfig { get => _usbConfig; set => SetProperty(ref _usbConfig, value); }
    public string BasebandVersion { get => _basebandVersion; set => SetProperty(ref _basebandVersion, value); }
    public string Imsi { get => _imsi; set => SetProperty(ref _imsi, value); }
    public string HardwareLock { get => _hardwareLock; set => SetProperty(ref _hardwareLock, value); }
    public string RawDevConInfo { get => _rawDevConInfo; set => SetProperty(ref _rawDevConInfo, value); }

    public string SelinuxStatus { get => _selinuxStatus; set => SetProperty(ref _selinuxStatus, value); }
    public string RsuAppStatus { get => _rsuAppStatus; set => SetProperty(ref _rsuAppStatus, value); }
    public string TsdaemonStatus { get => _tsdaemonStatus; set => SetProperty(ref _tsdaemonStatus, value); }
    public string Port2600Status { get => _port2600Status; set => SetProperty(ref _port2600Status, value); }
    public string RetailAgentStatus { get => _retailAgentStatus; set => SetProperty(ref _retailAgentStatus, value); }
    public string AtChannelStatus { get => _atChannelStatus; set => SetProperty(ref _atChannelStatus, value); }
    public string SimCardStatus { get => _simCardStatus; set => SetProperty(ref _simCardStatus, value); }
    public string NetworkLockStatus { get => _networkLockStatus; set => SetProperty(ref _networkLockStatus, value); }

    public string SelectedCsc { get => _selectedCsc; set => SetProperty(ref _selectedCsc, value); }
    public int SelectedMethodIndex
    {
        get => _selectedMethodIndex;
        set
        {
            if (SetProperty(ref _selectedMethodIndex, value))
            {
                OnPropertyChanged(nameof(IsMethod1Selected));
                OnPropertyChanged(nameof(IsMethod2Selected));
            }
        }
    }

    public bool IsMethod1Selected
    {
        get => _selectedMethodIndex == 0;
        set { if (value && _selectedMethodIndex != 0) SelectedMethodIndex = 0; }
    }

    public bool IsMethod2Selected
    {
        get => _selectedMethodIndex == 1;
        set { if (value && _selectedMethodIndex != 1) SelectedMethodIndex = 1; }
    }

    public string ArtifactsDirectory { get => _artifactsDirectory; set => SetProperty(ref _artifactsDirectory, value); }
    public bool UseEncryptedSession { get => _useEncryptedSession; set => SetProperty(ref _useEncryptedSession, value); }

    public string CpinState { get => _cpinState; set => SetProperty(ref _cpinState, value); }
    public string NetworkLockPn { get => _networkLockPn; set => SetProperty(ref _networkLockPn, value); }
    public string NetworkSubsetLockPu { get => _networkSubsetLockPu; set => SetProperty(ref _networkSubsetLockPu, value); }
    public string ServiceProviderLockPp { get => _serviceProviderLockPp; set => SetProperty(ref _serviceProviderLockPp, value); }
    public string CorporateLockPc { get => _corporateLockPc; set => SetProperty(ref _corporateLockPc, value); }
    public string SimPinLockSc { get => _simPinLockSc; set => SetProperty(ref _simPinLockSc, value); }
    public string SimLockSummary { get => _simLockSummary; set => SetProperty(ref _simLockSummary, value); }
    public bool IsSimFullyUnlocked { get => _isSimFullyUnlocked; set => SetProperty(ref _isSimFullyUnlocked, value); }

    public string MccMncSearchQuery
    {
        get => _mccMncSearchQuery;
        set
        {
            if (SetProperty(ref _mccMncSearchQuery, value))
            {
                FilterCarriers(value);
            }
        }
    }

    public MccMncEntry? SelectedCarrier
    {
        get => _selectedCarrier;
        set
        {
            if (SetProperty(ref _selectedCarrier, value) && value != null)
            {
                CustomMcc = value.Mcc;
                CustomMnc = value.Mnc;
                CalculatedBcdPayload = value.BcdPayload;
            }
        }
    }

    public string CustomMcc
    {
        get => _customMcc;
        set
        {
            if (SetProperty(ref _customMcc, value))
            {
                CalculatedBcdPayload = MccMncDatabase.EncodeMccMncToBcd(_customMcc, _customMnc);
            }
        }
    }

    public string CustomMnc
    {
        get => _customMnc;
        set
        {
            if (SetProperty(ref _customMnc, value))
            {
                CalculatedBcdPayload = MccMncDatabase.EncodeMccMncToBcd(_customMcc, _customMnc);
            }
        }
    }

    public string CalculatedBcdPayload { get => _calculatedBcdPayload; set => SetProperty(ref _calculatedBcdPayload, value); }

    public string SecuritySeed { get => _securitySeed; set => SetProperty(ref _securitySeed, value); }
    public string TerminalInput { get => _terminalInput; set => SetProperty(ref _terminalInput, value); }

    public bool IsBusy { get => _isBusy; set => SetProperty(ref _isBusy, value); }
    public string StatusText { get => _statusText; set => SetProperty(ref _statusText, value); }
    public double ProgressValue { get => _progressValue; set => SetProperty(ref _progressValue, value); }

    // --- Commands ---
    public ICommand ReadInfoCommand { get; }
    public ICommand RefreshTelemetryCommand { get; }
    public ICommand ChangeCscCommand { get; }
    public ICommand ExecuteMethod2Command { get; }
    public ICommand QuerySimLocksCommand { get; }
    public ICommand BoostUnlockCommand { get; }
    public ICommand CarrierRelockCommand { get; }
    public ICommand RadioRebootCommand { get; }
    public ICommand RadioOffCommand { get; }
    public ICommand RadioOnCommand { get; }
    public ICommand RadioFactoryResetCommand { get; }
    public ICommand PhoneRebootCommand { get; }
    public ICommand QuerySecuritySeedCommand { get; }
    public ICommand SendTerminalCommand { get; }
    public ICommand QuickAtCommand { get; }
    public ICommand ClearLogCommand { get; }
    public ICommand CopyLogCommand { get; }
    public ICommand ExportLogCommand { get; }

    public MainWindowViewModel(
        ISamsungCscWorkflow cscWorkflow,
        IDeviceTelemetryWorkflow? telemetryWorkflow = null,
        ISimLockWorkflow? simLockWorkflow = null,
        IBoostSprintWorkflow? boostWorkflow = null,
        IRadioControlWorkflow? radioWorkflow = null,
        IInteractiveAtWorkflow? interactiveWorkflow = null)
    {
        _cscWorkflow = cscWorkflow ?? throw new ArgumentNullException(nameof(cscWorkflow));

        var adbClient = new AdbRawSocketClient();
        var atSession = new PersistentAtBridgeSession(adbClient);

        _telemetryWorkflow = telemetryWorkflow ?? new DeviceTelemetryWorkflow(adbClient, atSession);
        _simLockWorkflow = simLockWorkflow ?? new SimLockWorkflow(adbClient, atSession);
        _boostWorkflow = boostWorkflow ?? new BoostSprintWorkflow(adbClient, atSession);
        _radioWorkflow = radioWorkflow ?? new RadioControlWorkflow(adbClient, atSession);
        _interactiveWorkflow = interactiveWorkflow ?? new InteractiveAtWorkflow(adbClient, atSession);

        // Bind Workflow Events
        RegisterLogHandler(_cscWorkflow.OnLog);
        RegisterLogHandler(_telemetryWorkflow.OnLog);
        RegisterLogHandler(_simLockWorkflow.OnLog);
        RegisterLogHandler(_boostWorkflow.OnLog);
        RegisterLogHandler(_radioWorkflow.OnLog);
        RegisterLogHandler(_interactiveWorkflow.OnLog);

        _cscWorkflow.OnStepChanged += HandleStepProgress;
        _boostWorkflow.OnStepChanged += HandleStepProgress;

        _telemetryWorkflow.OnDeviceInfoUpdated += UpdateDeviceFields;
        _cscWorkflow.OnDeviceInfoUpdated += UpdateDeviceFields;

        // Initialize Commands
        ReadInfoCommand = new RelayCommand(async _ => await ExecuteReadInfoAsync(), _ => !IsBusy);
        RefreshTelemetryCommand = new RelayCommand(async _ => await ExecuteReadInfoAsync(), _ => !IsBusy);
        ChangeCscCommand = new RelayCommand(async _ => await ExecuteChangeCscAsync(), _ => !IsBusy);
        ExecuteMethod2Command = new RelayCommand(async _ => await ExecuteMethod2Async(), _ => !IsBusy);
        QuerySimLocksCommand = new RelayCommand(async _ => await ExecuteQuerySimLocksAsync(), _ => !IsBusy);
        BoostUnlockCommand = new RelayCommand(async _ => await ExecuteBoostUnlockAsync(), _ => !IsBusy);
        CarrierRelockCommand = new RelayCommand(async _ => await ExecuteCarrierRelockAsync(), _ => !IsBusy);
        RadioRebootCommand = new RelayCommand(async _ => await ExecuteRadioActionAsync("REBOOT"), _ => !IsBusy);
        RadioOffCommand = new RelayCommand(async _ => await ExecuteRadioActionAsync("OFF"), _ => !IsBusy);
        RadioOnCommand = new RelayCommand(async _ => await ExecuteRadioActionAsync("ON"), _ => !IsBusy);
        RadioFactoryResetCommand = new RelayCommand(async _ => await ExecuteRadioActionAsync("FACTORST"), _ => !IsBusy);
        PhoneRebootCommand = new RelayCommand(async _ => await ExecuteRadioActionAsync("SWRESET"), _ => !IsBusy);
        QuerySecuritySeedCommand = new RelayCommand(async _ => await ExecuteQuerySecuritySeedAsync(), _ => !IsBusy);
        SendTerminalCommand = new RelayCommand(async _ => await ExecuteTerminalCommandAsync(), _ => !IsBusy && !string.IsNullOrWhiteSpace(TerminalInput));
        QuickAtCommand = new RelayCommand(async param => await ExecuteQuickAtAsync(param as string), _ => !IsBusy);

        ClearLogCommand = new RelayCommand(_ => LogItems.Clear());
        CopyLogCommand = new RelayCommand(_ => CopyLogToClipboard());
        ExportLogCommand = new RelayCommand(_ => ExportLogToFile());

        // Initialize Carrier Database
        LoadCarriers();
    }

    private void RegisterLogHandler(Action<string>? eventHook)
    {
        if (eventHook != null)
        {
            eventHook += msg =>
            {
                Application.Current?.Dispatcher.Invoke(() =>
                {
                    LogItems.Add(ConsoleLogItem.FromRawString(msg));
                });
            };
        }
    }

    private void HandleStepProgress(WorkflowStepProgress prog)
    {
        Application.Current?.Dispatcher.Invoke(() =>
        {
            ProgressValue = prog.ProgressPercentage;
            StatusText = $"Step {prog.StepNumber}/{prog.TotalSteps}: {prog.StepName} - {prog.StatusMessage}";
        });
    }

    private void UpdateDeviceFields(DeviceInfo info)
    {
        Application.Current?.Dispatcher.Invoke(() =>
        {
            Model = info.Model;
            Serial = info.Serial;
            Imei = info.Imei;
            CurrentCsc = info.CurrentCsc;
            ProductCode = info.ProductCode;
            BuildNumber = info.BuildNumber;
            AndroidVersion = info.AndroidVersion;
            SecurityPatch = info.SecurityPatch;
            UsbConfig = info.UsbConfig;
            SelinuxStatus = info.IsSelinuxEnforcing ? "Enforcing" : "Permissive";
            RsuAppStatus = info.HasRsuApp ? "Carrier Lock Detected" : "Clean";
            TsdaemonStatus = info.HasTsdaemon ? "Running" : "Clean";
            Port2600Status = info.Port2600Available ? "Connected uid=1000" : "Unavailable";
            RetailAgentStatus = info.RetailAgentVersion;
            AtChannelStatus = info.ActiveAtChannel;
            SimCardStatus = info.SimCardStatus;
            NetworkLockStatus = info.NetworkLockStatus;
            BasebandVersion = info.BasebandVersion;
            Imsi = info.Imsi;
            HardwareLock = info.HardwareLock;
            RawDevConInfo = info.RawDevConInfo;
        });
    }

    private void LoadCarriers()
    {
        try
        {
            var list = MccMncDatabase.GetAllEntries();
            AvailableCarriers.Clear();
            foreach (var item in list.Take(50))
            {
                AvailableCarriers.Add(item);
            }
            if (AvailableCarriers.Count > 0)
            {
                SelectedCarrier = AvailableCarriers[0];
            }
        }
        catch
        {
            // Fallback will load on search
        }
    }

    private void FilterCarriers(string query)
    {
        var filtered = MccMncDatabase.Search(query);
        AvailableCarriers.Clear();
        foreach (var item in filtered.Take(50))
        {
            AvailableCarriers.Add(item);
        }
    }

    private async Task ExecuteReadInfoAsync()
    {
        IsBusy = true;
        StatusText = "Reading comprehensive device telemetry...";
        ProgressValue = 10;
        try
        {
            var info = await _telemetryWorkflow.ReadDeepTelemetryAsync();
            UpdateDeviceFields(info);
            ProgressValue = 100;
            StatusText = $"Telemetry read successfully for {info.Model}.";
        }
        catch (Exception ex)
        {
            StatusText = $"Telemetry read failed: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteChangeCscAsync()
    {
        string cscCode = ExtractCscCode(SelectedCsc);
        IsBusy = true;
        StatusText = $"Initiating CSC change to {cscCode}...";
        ProgressValue = 0;

        try
        {
            CscChangeResult result;
            if (IsMethod2Selected)
            {
                result = await _cscWorkflow.ExecuteMethod2Async(cscCode, ArtifactsDirectory, UseEncryptedSession);
            }
            else
            {
                result = await _cscWorkflow.ExecuteCscChangeAsync(cscCode, UseEncryptedSession);
            }

            if (result.Success)
            {
                StatusText = $"Success: CSC changed from {result.OldCsc} to {result.NewCsc} in {result.Duration.TotalSeconds:F1}s.";
                CurrentCsc = result.NewCsc;
            }
            else
            {
                StatusText = $"CSC Change Error: {result.Message}";
            }
        }
        catch (Exception ex)
        {
            StatusText = $"Execution error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteMethod2Async()
    {
        SelectedMethodIndex = 1;
        await ExecuteChangeCscAsync();
    }

    private async Task ExecuteQuerySimLocksAsync()
    {
        IsBusy = true;
        StatusText = "Querying 3GPP SIM & Network personalization locks...";
        try
        {
            var lockInfo = await _simLockWorkflow.QuerySimLocksAsync();
            CpinState = lockInfo.CpinState;
            NetworkLockPn = lockInfo.NetworkLock;
            NetworkSubsetLockPu = lockInfo.NetworkSubsetLock;
            ServiceProviderLockPp = lockInfo.ServiceProviderLock;
            CorporateLockPc = lockInfo.CorporateLock;
            SimPinLockSc = lockInfo.SimPinLock;
            SimLockSummary = lockInfo.Summary;
            IsSimFullyUnlocked = lockInfo.IsFullyUnlocked;
            NetworkLockStatus = lockInfo.NetworkLock;
            SimCardStatus = lockInfo.CpinState;
            StatusText = $"SIM Locks: {lockInfo.Summary}";
        }
        catch (Exception ex)
        {
            StatusText = $"SIM lock query failed: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteBoostUnlockAsync()
    {
        IsBusy = true;
        StatusText = "Executing Sprint / Boost Mobile unlock sequence (/boost)...";
        ProgressValue = 0;

        try
        {
            var res = await _boostWorkflow.ExecuteBoostUnlockAsync();
            if (res.Success)
            {
                StatusText = $"Success: Sprint / Boost unlocked in {res.Duration.TotalSeconds:F1}s.";
                NetworkLockStatus = "UNLOCKED (0)";
                NetworkLockPn = "UNLOCKED (0)";
                IsSimFullyUnlocked = true;
            }
            else
            {
                StatusText = $"Sprint/Boost Unlock Failed: {res.Message}";
            }
        }
        catch (Exception ex)
        {
            StatusText = $"Sprint/Boost unlock error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteCarrierRelockAsync()
    {
        IsBusy = true;
        StatusText = $"Applying carrier relock for MCC={CustomMcc}, MNC={CustomMnc} (BCD: {CalculatedBcdPayload})...";
        ProgressValue = 0;

        try
        {
            string carrierName = SelectedCarrier?.Network ?? "Carrier Profile";
            var res = await _boostWorkflow.ExecuteCarrierRelockAsync(CustomMcc, CustomMnc, carrierName);
            if (res.Success)
            {
                StatusText = $"Success: Relock applied for {res.NetworkName} (BCD: {res.BcdPayload}) in {res.Duration.TotalSeconds:F1}s.";
            }
            else
            {
                StatusText = $"Carrier Relock Failed: {res.Message}";
            }
        }
        catch (Exception ex)
        {
            StatusText = $"Carrier relock error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteRadioActionAsync(string action)
    {
        IsBusy = true;
        StatusText = $"Executing radio action: {action}...";

        try
        {
            var res = await _radioWorkflow.ExecuteRadioActionAsync(action);
            StatusText = res.Success
                ? $"Radio '{action}' completed successfully ({res.Response})."
                : $"Radio action failed: {res.Message}";
        }
        catch (Exception ex)
        {
            StatusText = $"Radio action error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteQuerySecuritySeedAsync()
    {
        IsBusy = true;
        StatusText = "Querying modem security challenge seed (AT+AKSEEDNO=1,0)...";

        try
        {
            var res = await _radioWorkflow.QuerySecuritySeedAsync();
            SecuritySeed = res.ChallengeSeed;
            StatusText = res.Success ? "Security seed queried successfully." : $"Seed query failed: {res.Message}";
        }
        catch (Exception ex)
        {
            StatusText = $"Security seed error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteTerminalCommandAsync()
    {
        if (string.IsNullOrWhiteSpace(TerminalInput)) return;

        string cmd = TerminalInput.Trim();
        IsBusy = true;
        StatusText = $"Sending terminal command: {cmd}...";

        try
        {
            string resp = await _interactiveWorkflow.ExecuteRawCommandAsync(cmd);
            StatusText = $"Command finished. Response length: {resp.Length} chars.";
        }
        catch (Exception ex)
        {
            StatusText = $"Command error: {ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteQuickAtAsync(string? atCmd)
    {
        if (string.IsNullOrWhiteSpace(atCmd)) return;
        TerminalInput = atCmd;
        await ExecuteTerminalCommandAsync();
    }

    private static string ExtractCscCode(string cscSelection)
    {
        if (string.IsNullOrWhiteSpace(cscSelection)) return "XAA";
        string trimmed = cscSelection.Trim();
        return trimmed.Length >= 3 ? trimmed.Substring(0, 3).ToUpperInvariant() : trimmed.ToUpperInvariant();
    }

    private void CopyLogToClipboard()
    {
        try
        {
            string fullLog = string.Join(Environment.NewLine, LogItems.Select(i => $"[{i.Timestamp}] {i.Direction} {i.Message}"));
            Clipboard.SetText(fullLog);
            StatusText = "Console log copied to clipboard.";
        }
        catch (Exception ex)
        {
            StatusText = $"Copy failed: {ex.Message}";
        }
    }

    private void ExportLogToFile()
    {
        try
        {
            string fullLog = string.Join(Environment.NewLine, LogItems.Select(i => $"[{i.Timestamp}] {i.Direction} {i.Message}"));
            string exportPath = Path.Combine(AppContext.BaseDirectory, $"GsmTools_ExecutionLog_{DateTime.Now:yyyyMMdd_HHmmss}.log");
            File.WriteAllText(exportPath, fullLog);
            StatusText = $"Log exported to: {Path.GetFileName(exportPath)}";
        }
        catch (Exception ex)
        {
            StatusText = $"Export failed: {ex.Message}";
        }
    }
}
