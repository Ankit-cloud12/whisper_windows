# WhisperApp Release Checklist

## Pre-Release Preparation

### Code Quality
- [ ] All tests pass (`run-tests.bat`)
- [ ] No compiler warnings in release build
- [ ] Code review completed for all changes
- [ ] Memory leak check performed
- [ ] Performance benchmarks meet targets

### Version Updates
- [ ] Update version in `VERSION` file
- [ ] Update version in `CMakeLists.txt`
- [ ] Update version in `installer.nsi`
- [ ] Update version in About dialog
- [ ] Update copyright year if needed

### Documentation
- [ ] Update `CHANGELOG.md` with all changes
- [ ] Update `README.md` if features changed
- [ ] Update `USER_GUIDE.md` for new features
- [ ] Update `INSTALL.md` for new requirements
- [ ] Review and update API documentation
- [ ] Update screenshots if UI changed

### Localization
- [ ] All strings are translatable
- [ ] Translation files updated (`lupdate`)
- [ ] Translations reviewed by native speakers
- [ ] Translation files compiled (`lrelease`)

## Build Process

### Clean Build
- [ ] Delete all build directories
- [ ] Clean CMake cache
- [ ] Fresh checkout from repository

### Release Build
- [ ] Run `build-release.bat`
- [ ] Build completes without errors
- [ ] All dependencies collected
- [ ] Size is reasonable (<100MB installer)

### Portable Build
- [ ] Run `build-portable.bat`
- [ ] Portable package created
- [ ] Self-extracting archive created (optional)

### Code Signing
- [ ] Executable signed with valid certificate
- [ ] Installer signed with valid certificate
- [ ] Verify signatures with `signtool verify`
- [ ] No security warnings on download

## Testing

### Installation Testing
- [ ] Clean install on Windows 10
- [ ] Clean install on Windows 11
- [ ] Upgrade from previous version
- [ ] Uninstall removes all files
- [ ] Silent installation works
- [ ] File associations work (.wsp files)

### Portable Testing
- [ ] Runs from USB drive
- [ ] Settings saved in portable directory
- [ ] No registry entries created
- [ ] Works alongside installed version

### Functional Testing
- [ ] Recording works with default device
- [ ] All audio formats supported
- [ ] Transcription accuracy acceptable
- [ ] All languages work correctly
- [ ] Hotkeys function globally
- [ ] System tray integration works
- [ ] Auto-update check works

### Model Testing
- [ ] Base model included in installer
- [ ] Model download works
- [ ] All model sizes load correctly
- [ ] Model switching works

### UI Testing
- [ ] All dialogs open correctly
- [ ] No visual glitches
- [ ] Theme switching works
- [ ] High DPI displays supported
- [ ] Accessibility features work

### Performance Testing
- [ ] Startup time < 3 seconds
- [ ] Memory usage < 200MB idle
- [ ] CPU usage minimal when idle
- [ ] No memory leaks over time

## Distribution Preparation

### Package Creation
- [ ] Installer package created
- [ ] Portable ZIP created  
- [ ] Self-extracting EXE created (optional)
- [ ] All packages under size limits

### Checksum Generation
- [ ] SHA256 checksums generated
- [ ] Checksums verified manually
- [ ] Checksum file formatted correctly

### Virus Scanning
- [ ] Scan with Windows Defender
- [ ] Scan with online VirusTotal
- [ ] No false positives
- [ ] Add to whitelist if needed

## Release Publication

### GitHub Release
- [ ] Create release draft
- [ ] Use release template
- [ ] Tag version correctly (v1.0.0)
- [ ] Upload all packages
- [ ] Upload checksum file
- [ ] Set as pre-release if beta

### Release Notes
- [ ] Highlight major features
- [ ] List all bug fixes
- [ ] Include breaking changes
- [ ] Add upgrade instructions
- [ ] Thank contributors

### Update Infrastructure
- [ ] Update manifest for auto-updater
- [ ] Update download links
- [ ] Update website if applicable
- [ ] Update documentation links

## Post-Release

### Monitoring
- [ ] Monitor issue tracker
- [ ] Check crash reports
- [ ] Monitor download statistics
- [ ] Respond to user feedback

### Communication
- [ ] Announce on social media
- [ ] Post in relevant forums
- [ ] Email announcement list
- [ ] Update project status

### Backup
- [ ] Archive release builds
- [ ] Backup signing certificates
- [ ] Save build environment
- [ ] Document build process

## Rollback Plan

### If Issues Found
- [ ] Identify severity of issues
- [ ] Prepare hotfix if critical
- [ ] Update release notes
- [ ] Communicate with users
- [ ] Plan next patch release

### Emergency Contacts
- Project Lead: @username
- Lead Developer: @username  
- QA Lead: @username
- Community Manager: @username

---

**Release Sign-off**

- [ ] Project Lead approval
- [ ] QA Lead approval
- [ ] Development Lead approval

Release Date: _______________
Released By: _______________
Version: _______________