import { app, BrowserWindow, shell, systemPreferences } from 'electron';

// disable security warnings for live reload
process.env['ELECTRON_DISABLE_SECURITY_WARNINGS'] = 'true';

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require('electron-squirrel-startup')) {
  app.quit();
}

function createWindow() {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    height: 800,
    width: 1200,
    // fullscreen: true,
    webPreferences: {
      // contextIsolation needs to be false to run video-module in renderer
      contextIsolation: false,
      nativeWindowOpen: true,
      nodeIntegrationInWorker: true,
      nodeIntegration: true,

      // devTools: !app.isPackaged,
    },
  });

  if (process.platform === 'darwin') {
    systemPreferences.askForMediaAccess('camera');
  }

  // and load the index.html of the app.
  mainWindow.loadURL(MAIN_WINDOW_WEBPACK_ENTRY);

  // TODO: deep linking
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url);
    return { action: 'deny' };
  });

  mainWindow.webContents.on('will-navigate', (e) => e.preventDefault());

  mainWindow.webContents.on('did-frame-finish-load', () => {
    // Open the DevTools.
    if (!app.isPackaged) {
      mainWindow.webContents.openDevTools();
    }
  });
}

// This method will be called when Electron has finished initialization and is
// ready to create browser windows. Some APIs can only be used after this event
// occurs.
app.on('ready', () => {
  createWindow();
});

// Quit when all windows are closed, except on macOS. There, it's common for
// applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  // Disabled for now
  // if (process.platform !== 'darwin') {
  //   app.quit();
  // }

  app.quit();
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the dock icon is
  // clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and import them here.
