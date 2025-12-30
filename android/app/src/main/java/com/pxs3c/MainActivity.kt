package com.pxs3c

import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts

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
    
    private lateinit var filePickerLauncher: ActivityResultLauncher<Intent>
    private var gameLoaded = false
    private lateinit var surfaceView: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        surfaceView = findViewById<SurfaceView>(R.id.surfaceView)
        val btnSettings = findViewById<Button>(R.id.btnSettings)
        val btnLoadGame = findViewById<Button>(R.id.btnLoadGame)
        
        // Setup file picker result handler
        filePickerLauncher = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == RESULT_OK) {
                val filePath = result.data?.getStringExtra("FILE_PATH")
                if (filePath != null) {
                    loadGame(filePath)
                }
            }
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
            openFilePicker()
        }
    }
    
    private fun openFilePicker() {
        val intent = Intent(this, FilePickerActivity::class.java)
        filePickerLauncher.launch(intent)
    }
    
    private fun loadGame(filePath: String) {
        if (nativeLoadGame(filePath)) {
            gameLoaded = true
            android.widget.Toast.makeText(
                this,
                "Game loaded: ${java.io.File(filePath).name}",
                android.widget.Toast.LENGTH_SHORT
            ).show()
        } else {
            android.widget.Toast.makeText(
                this,
                "Failed to load game",
                android.widget.Toast.LENGTH_SHORT
            ).show()
        }
    }
    
    private fun startFrameLoop() {
        surfaceView.handler?.post(object : Runnable {
            override fun run() {
                val delay = nativeTickFrame()
                surfaceView.handler?.postDelayed(this, delay.toLong())
            }
        })
    }

    override fun onDestroy() {
        nativeShutdown()
        super.onDestroy()
    }
}
