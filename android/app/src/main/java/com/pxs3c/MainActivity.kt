package com.pxs3c

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button

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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val sv = findViewById<SurfaceView>(R.id.surfaceView)
        val btn = findViewById<Button>(R.id.btnSettings)

        sv.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                nativeInit()
                nativeAttachSurface(holder.surface)
                nativeSetTargetFps(60)
                nativeSetVsync(true)
                sv.handler?.post(object : Runnable {
                    override fun run() {
                        val delay = nativeTickFrame()
                        sv.handler?.postDelayed(this, delay.toLong())
                    }
                })
            }
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                nativeResize(width, height)
            }
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                nativeShutdown()
            }
        })

        btn.setOnClickListener {
            startActivity(android.content.Intent(this, SettingsActivity::class.java))
        }
    }

    override fun onDestroy() {
        nativeShutdown()
        super.onDestroy()
    }
}
