using System.Windows;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.ViewModels;
using GsmTools.Desktop.Workflows;

namespace GsmTools.Desktop;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();

        // Instantiate core architectural services
        var adbClient = new AdbRawSocketClient();
        var atSession = new PersistentAtBridgeSession(adbClient);

        var cscWorkflow = new SamsungCscWorkflow(adbClient, atSession);
        var telemetryWorkflow = new DeviceTelemetryWorkflow(adbClient, atSession);
        var simLockWorkflow = new SimLockWorkflow(adbClient, atSession);
        var boostWorkflow = new BoostSprintWorkflow(adbClient, atSession);
        var radioWorkflow = new RadioControlWorkflow(adbClient, atSession);
        var interactiveWorkflow = new InteractiveAtWorkflow(adbClient, atSession);

        DataContext = new MainWindowViewModel(
            cscWorkflow,
            telemetryWorkflow,
            simLockWorkflow,
            boostWorkflow,
            radioWorkflow,
            interactiveWorkflow
        );
    }
}