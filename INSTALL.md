# WhisperApp Installation Guide

## System Requirements

### Minimum Requirements
- **Operating System**: Windows 10 64-bit (version 1809 or later)
- **Processor**: x64 processor with AVX support
- **Memory**: 4 GB RAM
- **Storage**: 500 MB available space (plus space for models)
- **Graphics**: DirectX 11 compatible
- **Network**: Internet connection for model downloads

### Recommended Requirements
- **Operating System**: Windows 11 64-bit
- **Processor**: Intel Core i5 or AMD Ryzen 5 (or better)
- **Memory**: 8 GB RAM or more
- **Storage**: 2 GB available space
- **Graphics**: Dedicated GPU with CUDA support (for faster processing)

### Additional Requirements
- Microsoft Visual C++ Redistributable 2022 (included in installer)
- .NET Framework 4.8 (usually pre-installed on Windows 10/11)

## Installation Methods

### Method 1: Installer (Recommended)

1. **Download the installer**
   - Download `WhisperApp-1.0.0-Setup.exe` from the releases page
   - Verify the SHA256 checksum (see below)

2. **Run the installer**
   - Right-click the installer and select "Run as administrator"
   - Follow the installation wizard
   - Choose installation directory (default: `C:\Program Files\WhisperApp`)
   - Select components:
     - WhisperApp Core (required)
     - Visual C++ Runtime (recommended)
     - File Associations (optional)

3. **Complete installation**
   - Click "Install" to begin
   - Wait for installation to complete
   - Launch WhisperApp from Start Menu or Desktop shortcut

### Method 2: Portable Version

1. **Download the portable package**
   - Download `WhisperApp-1.0.0-portable-win64.zip`
   - Extract to any location (e.g., USB drive, Documents folder)

2. **Run WhisperApp**
   - Navigate to the extracted folder
   - Run `WhisperApp-Portable.bat`
   - All settings will be stored in the `settings` subfolder

### Method 3: Silent Installation (Enterprise)

For automated deployment, use silent installation:

```batch
WhisperApp-1.0.0-Setup.exe /S /D=C:\Program Files\WhisperApp
```

Parameters:
- `/S` - Silent mode
- `/D=path` - Installation directory (must be last parameter)

## Post-Installation Setup

### 1. Download Language Models

On first launch, WhisperApp will prompt to download language models:

1. Open WhisperApp
2. Go to Settings → Models
3. Click "Download Models"
4. Select desired models:
   - **base.en** (140 MB) - English only, fastest
   - **small** (460 MB) - Multilingual, good balance
   - **medium** (1.5 GB) - Better accuracy
   - **large** (3.1 GB) - Best accuracy

### 2. Configure Audio Devices

1. Go to Settings → Audio
2. Select input device (microphone)
3. Test recording levels
4. Adjust noise suppression if needed

### 3. Set Up Hotkeys

1. Go to Settings → Hotkeys
2. Configure global shortcuts:
   - Start/Stop Recording: `Ctrl+Alt+R`
   - Quick Transcribe: `Ctrl+Alt+T`
   - Show/Hide Window: `Ctrl+Alt+W`

## Troubleshooting

### Common Issues

#### "MSVCP140.dll not found" Error
- **Solution**: Install Visual C++ Redistributable
- Run `vc_redist.x64.exe` from installation directory
- Or download from Microsoft website

#### "Failed to initialize audio device"
- **Check**: Microphone permissions in Windows Settings
- Go to Settings → Privacy → Microphone
- Enable microphone access for desktop apps

#### "Model download failed"
- **Check**: Internet connection and firewall settings
- **Alternative**: Download models manually from GitHub
- Place in `%APPDATA%\WhisperApp\models\`

#### Application crashes on startup
1. Check Windows Event Viewer for error details
2. Try running in compatibility mode
3. Disable antivirus temporarily
4. Reinstall with administrator privileges

### Permission Issues

If you encounter permission errors:

1. **Run as Administrator**
   - Right-click WhisperApp.exe
   - Select "Run as administrator"

2. **Check folder permissions**
   - Ensure write access to:
     - `%APPDATA%\WhisperApp`
     - Installation directory

3. **Windows Defender**
   - Add WhisperApp to exclusions
   - Path: Windows Security → Virus & threat protection → Exclusions

## Verifying Installation

### Check Installation
1. Open Command Prompt
2. Navigate to installation directory
3. Run: `WhisperApp.exe --version`
4. Should display: `WhisperApp version 1.0.0`

### Verify File Integrity

Use SHA256 checksums to verify downloads:

```batch
certutil -hashfile WhisperApp-1.0.0-Setup.exe SHA256
```

Compare with published checksums in `WhisperApp-SHA256.txt`

## Uninstallation

### Via Control Panel
1. Open Control Panel → Programs and Features
2. Find "WhisperApp"
3. Click "Uninstall"
4. Follow uninstallation wizard

### Via Command Line
```batch
"C:\Program Files\WhisperApp\uninst.exe"
```

### Manual Cleanup
After uninstallation, optionally remove:
- User data: `%APPDATA%\WhisperApp`
- Registry keys: `HKLM\Software\WhisperApp`

## Network Configuration

### Firewall Rules
WhisperApp requires network access for:
- Model downloads
- Update checks
- Online features

The installer automatically adds firewall rules. For manual configuration:

```batch
netsh advfirewall firewall add rule name="WhisperApp" dir=in action=allow program="C:\Program Files\WhisperApp\WhisperApp.exe" enable=yes
```

### Proxy Settings
For corporate environments with proxy:
1. Go to Settings → Network
2. Configure proxy settings
3. Enter proxy URL and credentials if required

## Support

### Getting Help
- **Documentation**: See USER_GUIDE.md
- **Issues**: GitHub Issues page
- **Community**: Discord server
- **Email**: support@whisperapp.example.com

### Reporting Issues
When reporting issues, include:
- WhisperApp version
- Windows version
- Error messages
- Steps to reproduce
- System specifications

## License

WhisperApp is distributed under the MIT License. See LICENSE file for details.
Third-party components have their own licenses - see NOTICE file.