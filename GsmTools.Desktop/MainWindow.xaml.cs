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
        var workflow = new SamsungCscWorkflow(adbClient, atSession);

        DataContext = new MainWindowViewModel(workflow);
    }
}