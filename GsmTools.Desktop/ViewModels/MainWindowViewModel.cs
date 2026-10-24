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
    private readonly ISamsungCscWorkflow _workflow;

    private string _model = "SM-S918U";
    private string _serial = "RFCX90GR21X";
    private string _imei = "355752920542633";
    private string _currentCsc = "ATT";
    private string _productCode = "SM-S918UZKVATT";
    private string _buildNumber = "S918USQS6DYG1";
    private string _androidVersion = "15";
    private string _securityPatch = "2025-07-01";
    private string _usbConfig = "mtp,adb";

    private string _selinuxStatus = "Enforcing";
    private string _rsuAppStatus = "Clean";
    private string _tsdaemonStatus = "Clean";
    private string _port2600Status = "Connected uid=1000";
    private string _retailAgentStatus = "5.59.6-main";
    private string _atChannelStatus = "/dev/umts_ipc0";
    private string _simCardStatus = "Ready";
    private string _networkLockStatus = "UNLOCKED (0)";

    private string _selectedCsc = "XAA";
    private int _selectedMethodIndex = 1; // Default to Method 2 as requested by user
    private string _artifactsDirectory = @"C:\Users\pc5\Documents\GitHub\gsmtools\2";
    private bool _useEncryptedSession = false;
    private bool _isBusy = false;
    private string _statusText = "Ready - Ready to orchestrate Samsung CSC operations (Method 1 & Method 2 supported).";
    private double _progressValue = 0;

    public ObservableCollection<string> AvailableCscs { get; } = new()
    {
        "XAA - USA Unbranded / Open",
        "USC - US Cellular",
        "TMB - T-Mobile USA",
        "ATT - AT&T USA",
        "VZW - Verizon USA",
        "GCF - Global Certification",
        "FKR - FKR USA",
        "XAG - USA Open Variant",
        "XAS - Sprint / T-Mobile USA",
        "XAB - USA Open",
        "EUX - European Open",
        "KOO - Korea Open"
    };

    public ObservableCollection<ConsoleLogItem> LogItems { get; } = new();

    public string Model { get => _model; set => SetProperty(ref _model, value); }
    public string Serial { get => _serial; set => SetProperty(ref _serial, value); }
    public string Imei { get => _imei; set => SetProperty(ref _imei, value); }
    public string CurrentCsc { get => _currentCsc; set => SetProperty(ref _currentCsc, value); }
    public string ProductCode { get => _productCode; set => SetProperty(ref _productCode, value); }
    public string BuildNumber { get => _buildNumber; set => SetProperty(ref _buildNumber, value); }
    public string AndroidVersion { get => _androidVersion; set => SetProperty(ref _androidVersion, value); }
    public string SecurityPatch { get => _securityPatch; set => SetProperty(ref _securityPatch, value); }
    public string UsbConfig { get => _usbConfig; set => SetProperty(ref _usbConfig, value); }

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
        set
        {
            if (value && _selectedMethodIndex != 0)
            {
                SelectedMethodIndex = 0;
            }
        }
    }

    public bool IsMethod2Selected
    {
        get => _selectedMethodIndex == 1;
        set
        {
            if (value && _selectedMethodIndex != 1)
            {
                SelectedMethodIndex = 1;
            }
        }
    }

    public string ArtifactsDirectory { get => _artifactsDirectory; set => SetProperty(ref _artifactsDirectory, value); }
    public bool UseEncryptedSession { get => _useEncryptedSession; set => SetProperty(ref _useEncryptedSession, value); }
    public bool IsBusy { get => _isBusy; set => SetProperty(ref _isBusy, value); }
    public string StatusText { get => _statusText; set => SetProperty(ref _statusText, value); }
    public double ProgressValue { get => _progressValue; set => SetProperty(ref _progressValue, value); }

    public ICommand ReadInfoCommand { get; }
    public ICommand ChangeCscCommand { get; }
    public ICommand ExecuteMethod2Command { get; }
    public ICommand ClearLogCommand { get; }
    public ICommand CopyLogCommand { get; }
    public ICommand ExportLogCommand { get; }

    public MainWindowViewModel(ISamsungCscWorkflow workflow)
    {
        _workflow = workflow ?? throw new ArgumentNullException(nameof(workflow));

        _workflow.OnLog += msg =>
        {
            Application.Current?.Dispatcher.Invoke(() =>
            {
                LogItems.Add(ConsoleLogItem.FromRawString(msg));
            });
        };

        _workflow.OnStepChanged += prog =>
        {
            Application.Current?.Dispatcher.Invoke(() =>
            {
                ProgressValue = prog.ProgressPercentage;
                StatusText = $"Step {prog.StepNumber}/{prog.TotalSteps}: {prog.StepName} - {prog.StatusMessage}";
            });
        };

        _workflow.OnDeviceInfoUpdated += UpdateDeviceInfo;

        ReadInfoCommand = new AsyncRelayCommand(ExecuteReadInfoAsync, () => !IsBusy);
        ChangeCscCommand = new AsyncRelayCommand(ExecuteChangeCscAsync, () => !IsBusy);
        ExecuteMethod2Command = new AsyncRelayCommand(ExecuteMethod2DirectAsync, () => !IsBusy);
        ClearLogCommand = new RelayCommand(() => LogItems.Clear());
        CopyLogCommand = new RelayCommand(ExecuteCopyLog);
        ExportLogCommand = new RelayCommand(ExecuteExportLog);

        // Initial welcome log item
        LogItems.Add(ConsoleLogItem.FromRawString($"[{DateTime.Now:HH:mm:ss.fff}] GsmTools.Desktop v2.0 Forensic Orchestrator Initialized. Ready for device communication."));
    }

    private async Task ExecuteReadInfoAsync()
    {
        IsBusy = true;
        StatusText = "Reading device telemetry...";
        ProgressValue = 10;
        try
        {
            await _workflow.ReadDeviceInfoAsync();
            StatusText = "Device telemetry acquired successfully.";
            ProgressValue = 100;
        }
        catch (Exception ex)
        {
            StatusText = $"Read Info Failed: {ex.Message}";
            LogItems.Add(ConsoleLogItem.FromRawString($"[{DateTime.Now:HH:mm:ss.fff}] ERROR: {ex.Message}"));
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteChangeCscAsync()
    {
        IsBusy = true;
        string methodName = SelectedMethodIndex == 1 ? "Method 2 (Exploit Escalation / 33.txt)" : "Method 1 (Standard Direct AT)";
        StatusText = $"Executing {methodName}...";
        ProgressValue = 0;
        try
        {
            // Extract the 3-letter code from combo text (e.g. "XAA - USA Unbranded" -> "XAA")
            string targetCode = SelectedCsc.Trim();
            if (targetCode.Contains('-'))
                targetCode = targetCode.Split('-')[0].Trim();

            CscChangeResult result = SelectedMethodIndex == 1
                ? await _workflow.ExecuteMethod2Async(targetCode, ArtifactsDirectory, UseEncryptedSession)
                : await _workflow.ExecuteCscChangeAsync(targetCode, UseEncryptedSession);

            if (result.Success)
            {
                StatusText = result.Message;
                CurrentCsc = result.NewCsc;
            }
            else
            {
                StatusText = $"CSC Change Failed: {result.Message}";
            }
        }
        catch (Exception ex)
        {
            StatusText = $"Execution Failed: {ex.Message}";
            LogItems.Add(ConsoleLogItem.FromRawString($"[{DateTime.Now:HH:mm:ss.fff}] ERROR: {ex.Message}"));
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task ExecuteMethod2DirectAsync()
    {
        SelectedMethodIndex = 1;
        await ExecuteChangeCscAsync();
    }

    private void UpdateDeviceInfo(DeviceInfo info)
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
        RsuAppStatus = info.HasRsuApp ? "Detected: rsuapp" : "Clean";
        TsdaemonStatus = info.HasTsdaemon ? "Detected: tsdaemon" : "Clean";
        Port2600Status = info.Port2600Available ? "Connected uid=1000" : "Refused / Inactive";
        RetailAgentStatus = string.IsNullOrWhiteSpace(info.RetailAgentVersion) ? "Not Installed" : info.RetailAgentVersion;
        AtChannelStatus = info.ActiveAtChannel;
        SimCardStatus = info.SimCardStatus;
        NetworkLockStatus = info.NetworkLockStatus;
    }

    private void ExecuteCopyLog()
    {
        var lines = LogItems.Select(item => $"[{item.Timestamp}] {item.Direction} {item.Message}");
        Clipboard.SetText(string.Join(Environment.NewLine, lines));
        StatusText = "Log copied to clipboard.";
    }

    private void ExecuteExportLog()
    {
        string filename = $"GsmTools_Log_{DateTime.Now:yyyyMMdd_HHmmss}.txt";
        string exportPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), filename);
        var lines = LogItems.Select(item => $"[{item.Timestamp}] {item.Direction} {item.Message}");
        File.WriteAllLines(exportPath, lines);
        StatusText = $"Log exported to Desktop: {filename}";
        LogItems.Add(ConsoleLogItem.FromRawString($"[{DateTime.Now:HH:mm:ss.fff}] Forensic log exported to: {exportPath}"));
    }
}
