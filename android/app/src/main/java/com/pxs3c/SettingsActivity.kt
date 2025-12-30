package com.pxs3c

import android.os.Bundle
import android.widget.Button
import android.widget.SeekBar
import android.widget.Switch
import android.widget.EditText
import androidx.appcompat.app.AppCompatActivity

class SettingsActivity : AppCompatActivity() {
    companion object {
        init { System.loadLibrary("pxs3c_jni") }
    }

    external fun nativeSetTargetFps(fps: Int)
    external fun nativeSetClearColor(r: Float, g: Float, b: Float)
    external fun nativeSetVsync(enabled: Boolean)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        val fpsSeek = findViewById<SeekBar>(R.id.fpsSeek)
        val vsyncSwitch = findViewById<Switch>(R.id.vsyncSwitch)
        val rEdit = findViewById<EditText>(R.id.editR)
        val gEdit = findViewById<EditText>(R.id.editG)
        val bEdit = findViewById<EditText>(R.id.editB)
        val applyBtn = findViewById<Button>(R.id.applyBtn)

        fpsSeek.max = 1 // 0->30, 1->60
        fpsSeek.progress = 1
        vsyncSwitch.isChecked = true
        rEdit.setText("0.03")
        gEdit.setText("0.03")
        bEdit.setText("0.08")

        applyBtn.setOnClickListener {
            val fps = if (fpsSeek.progress == 1) 60 else 30
            nativeSetTargetFps(fps)
            val r = rEdit.text.toString().toFloatOrNull() ?: 0.03f
            val g = gEdit.text.toString().toFloatOrNull() ?: 0.03f
            val b = bEdit.text.toString().toFloatOrNull() ?: 0.08f
            nativeSetClearColor(r, g, b)
            nativeSetVsync(vsyncSwitch.isChecked)
            finish()
        }
    }
}
