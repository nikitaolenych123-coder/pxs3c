package com.pxs3c

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.ParcelFileDescriptor
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import java.io.File
import java.io.FileOutputStream

class MainActivity : BaseActivity() {
    companion object {
        init {
            try {
                System.loadLibrary("pxs3c_jni")
                android.util.Log.i("PXS3C-JNI", "✓ Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("PXS3C-JNI", "✗ Failed to load pxs3c_jni: ${e.message}", e)
            } catch (e: Exception) {
                android.util.Log.e("PXS3C-JNI", "✗ Unexpected error loading native library: ${e.message}", e)
            }
        }
    }

    external fun nativeInit(): Boolean
    external fun nativeLoadGame(path: String): Boolean
    external fun nativeRunFrame()
    external fun nativeShutdown()
    external fun nativeAttachSurface(surface: android.view.Surface): Boolean
    external fun nativeSetTargetFps(fps: Int)
    external fun nativeTickFrame(): Int
    external fun nativeResize(width: Int, height: Int): Boolean
    external fun nativeSetClearColor(r: Float, g: Float, b: Float)
    external fun nativeSetVsync(enabled: Boolean)
    
    private lateinit var filePickerLauncher: ActivityResultLauncher<String>
    private var gameLoaded = false
    private var isRunning = false
    private lateinit var surfaceView: SurfaceView
    private lateinit var statusText: TextView
    private lateinit var fpsText: TextView
    private lateinit var btnStop: Button
    private lateinit var btnBootGame: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        // Use safe findViewById with automatic type handling
        surfaceView = findViewByIdSafe(R.id.surfaceView) ?: return
        statusText = findViewByIdSafe(R.id.statusText) ?: return
        fpsText = findViewByIdSafe(R.id.fpsText) ?: return
        val btnSettings: android.widget.ImageButton? = findViewByIdSafe(R.id.btnSettings)
        val btnLoadGame: Button? = findViewByIdSafe(R.id.btnLoadGame)
        btnBootGame = findViewByIdSafe(R.id.btnBootGame) ?: return
        btnStop = findViewByIdSafe(R.id.btnStop) ?: return
        val btnRefresh: android.widget.ImageButton? = findViewByIdSafe(R.id.btnRefresh)
        
        statusText.text = "✓ UI Initialized"
        android.util.Log.i("PXS3C-Main", "✓ onCreate started successfully")
    
        // Use SAF (Storage Access Framework) for file picking
        filePickerLauncher = registerForActivityResult(
            ActivityResultContracts.GetContent()
        ) { uri: Uri? ->
            uri?.let { loadGameFromUri(it) }
        }

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                try {
                    if (!nativeInit()) {
                        runOnUiThread {
                            statusText.text = "Failed to initialize emulator"
                            Toast.makeText(this@MainActivity, "Emulator init failed", Toast.LENGTH_LONG).show()
                        }
                        return
                    }
                    
                    if (!nativeAttachSurface(holder.surface)) {
                        runOnUiThread {
                            statusText.text = "Failed to attach surface"
                            Toast.makeText(this@MainActivity, "Surface attach failed", Toast.LENGTH_LONG).show()
                        }
                        return
                    }
                    
                    nativeSetTargetFps(60)
                    nativeSetVsync(true)
                    
                    runOnUiThread {
                        statusText.text = "Ready - Load a game to start"
                    }
                    
                    // Don't auto-start frame loop - wait for game to load
                } catch (e: Exception) {
                    runOnUiThread {
                        statusText.text = "Error: ${e.message}"
                        Toast.makeText(this@MainActivity, "Init error: ${e.message}", Toast.LENGTH_LONG).show()
                    }
                }
            }
            
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                try {
                    nativeResize(width, height)
                } catch (e: Exception) {
                    // Ignore resize errors
                }
            }
            
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                try {
                    stopFrameLoop()
                    nativeShutdown()
                } catch (e: Exception) {
                    // Ignore shutdown errors
                }
            }
        })

        btnSettings?.setOnClickListener {
            startActivity(Intent(this, SettingsActivity::class.java))
        }
        
        btnLoadGame?.setOnClickListener {
            statusText.text = "Select PKG/ISO/ELF file..."
            filePickerLauncher.launch("*/*")
        }
        
        btnBootGame.setOnClickListener {
            if (!gameLoaded) {
                Toast.makeText(this, "Please load a game first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (!isRunning) {
                startFrameLoop()
                btnStop.isEnabled = true
                btnBootGame.text = "Pause"
            } else {
                stopFrameLoop()
                btnBootGame.text = "Resume"
            }
        }
        
        btnStop.setOnClickListener {
            stopFrameLoop()
            gameLoaded = false
            btnStop.isEnabled = false
            btnBootGame.text = "Boot Game"
            statusText.text = "Stopped"
            fpsText.text = "FPS: 0"
        }
        
        btnRefresh?.setOnClickListener {
            // Refresh game list (placeholder)
            Toast.makeText(this, "Game list refresh (not implemented)", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun loadGameFromUri(uri: Uri) {
        Thread {
            try {
                val fileName = getFileName(uri) ?: "game.tmp"
                val tempFile = File(cacheDir, fileName)
                
                runOnUiThread {
                    statusText.text = "Loading: $fileName..."
                }
                
                // Buffered copy for performance
                contentResolver.openInputStream(uri)?.use { input ->
                    FileOutputStream(tempFile).buffered(16384).use { output ->
                        input.copyTo(output, 16384)
                    }
                }
                
                val success = nativeLoadGame(tempFile.absolutePath)
                
                runOnUiThread {
                    if (success) {
                        gameLoaded = true
                        statusText.text = "Game loaded: $fileName - Starting emulation..."
                        Toast.makeText(this, "Game loaded! Check logcat for compilation details", Toast.LENGTH_LONG).show()
                        
                        // Start frame loop after successful game load
                        startFrameLoop()
                    } else {
                        statusText.text = "Failed to load game - Check logcat for details"
                        Toast.makeText(this, "Failed to load game", Toast.LENGTH_SHORT).show()
                        tempFile.delete() // Cleanup on failure
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    statusText.text = "Error: ${e.message}"
                    Toast.makeText(this, "Error loading file: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }.start()
    }
    
    private fun getFileName(uri: Uri): String? {
        var name: String? = null
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
                if (nameIndex >= 0) {
                    name = cursor.getString(nameIndex)
                }
            }
        }
        return name ?: uri.lastPathSegment
    }
    
    private var frameHandler: android.os.Handler? = null
    private var frameRunnable: Runnable? = null
    
    private fun startFrameLoop() {
        if (isRunning) return
        isRunning = true
        statusText.text = "Emulator running"
        
        frameHandler = android.os.Handler(android.os.Looper.getMainLooper())
        frameRunnable = object : Runnable {
            override fun run() {
                if (!isRunning) return
                try {
                    val delay = nativeTickFrame().toLong().coerceIn(1L, 100L)
                    frameHandler?.postDelayed(this, delay)
                } catch (e: Exception) {
                    isRunning = false
                    runOnUiThread {
                        statusText.text = "Error: ${e.message}"
                    }
                }
            }
        }
        frameHandler?.post(frameRunnable!!)
    }
    
    private fun stopFrameLoop() {
        isRunning = false
        frameRunnable?.let { frameHandler?.removeCallbacks(it) }
        frameHandler = null
        frameRunnable = null
    }

    override fun onDestroy() {
        try {
            stopFrameLoop()
            nativeShutdown()
            // Cleanup cache
            cacheDir.listFiles()?.forEach { 
                try {
                    it.delete()
                } catch (e: Exception) {
                    // Ignore delete errors
                }
            }
        } catch (e: Exception) {
            // Ignore cleanup errors
        }
        super.onDestroy()
    }
    
    override fun onPause() {
        super.onPause()
        stopFrameLoop()
    }
    
    override fun onResume() {
        super.onResume()
        if (gameLoaded && !isRunning) {
            startFrameLoop()
        }
    }
}
