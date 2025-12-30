package com.pxs3c

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.ParcelFileDescriptor
import androidx.appcompat.app.AppCompatActivity
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {
    companion object {
        init {
            System.loadLibrary("pxs3c_jni")
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        surfaceView = findViewById(R.id.surfaceView)
        statusText = findViewById(R.id.statusText)
        val btnSettings = findViewById<Button>(R.id.btnSettings)
        val btnLoadGame = findViewById<Button>(R.id.btnLoadGame)
        
        // Use SAF (Storage Access Framework) for file picking
        filePickerLauncher = registerForActivityResult(
            ActivityResultContracts.GetContent()
        ) { uri: Uri? ->
            uri?.let { loadGameFromUri(it) }
        }

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                nativeInit()
                nativeAttachSurface(holder.surface)
                nativeSetTargetFps(60)
                nativeSetVsync(true)
                
                // Auto-start frame loop
                startFrameLoop()
            }
            
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                nativeResize(width, height)
            }
            
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                nativeShutdown()
            }
        })

        btnSettings.setOnClickListener {
            startActivity(Intent(this, SettingsActivity::class.java))
        }
        
        btnLoadGame.setOnClickListener {
            filePickerLauncher.launch("*/*") // Allow all file types
        }
    }
    
    private fun loadGameFromUri(uri: Uri) {
        try {
            // Copy URI content to temp file in cache dir
            val fileName = getFileName(uri) ?: "game.tmp"
            val tempFile = File(cacheDir, fileName)
            
            statusText.text = "Loading: $fileName..."
            
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(tempFile).use { output ->
                    input.copyTo(output)
                }
            }
            
            if (nativeLoadGame(tempFile.absolutePath)) {
                gameLoaded = true
                statusText.text = "Game loaded: $fileName"
                Toast.makeText(this, "Game loaded successfully", Toast.LENGTH_SHORT).show()
            } else {
                statusText.text = "Failed to load game"
                Toast.makeText(this, "Failed to load game", Toast.LENGTH_SHORT).show()
            }
        } catch (e: Exception) {
            statusText.text = "Error: ${e.message}"
            Toast.makeText(this, "Error loading file: ${e.message}", Toast.LENGTH_SHORT).show()
        }
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
    
    private fun startFrameLoop() {
        if (isRunning) return
        isRunning = true
        statusText.text = "Emulator running"
        surfaceView.handler?.post(object : Runnable {
            override fun run() {
                if (!isRunning) return
                val delay = nativeTickFrame()
                surfaceView.handler?.postDelayed(this, delay.toLong())
            }
        })
    }

    override fun onDestroy() {
        isRunning = false
        nativeShutdown()
        super.onDestroy()
    }
    
    override fun onPause() {
        super.onPause()
        isRunning = false
    }
    
    override fun onResume() {
        super.onResume()
        if (gameLoaded) {
            isRunning = true
            startFrameLoop()
        }
    }
}
