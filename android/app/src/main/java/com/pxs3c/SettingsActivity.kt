package com.pxs3c

import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.SwitchCompat

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
        val fpsLabel = findViewById<TextView>(R.id.fpsLabel)
        val vsyncSwitch = findViewById<SwitchCompat>(R.id.vsyncSwitch)
        val ppuDecoderSpinner = findViewById<Spinner>(R.id.ppuDecoderSpinner)
        val rEdit = findViewById<EditText>(R.id.editR)
        val gEdit = findViewById<EditText>(R.id.editG)
        val bEdit = findViewById<EditText>(R.id.editB)
        val applyBtn = findViewById<Button>(R.id.applyBtn)
        val cancelBtn = findViewById<Button>(R.id.cancelBtn)

        // Setup PPU Decoder spinner
        val ppuOptions = arrayOf("Interpreter (default)", "LLVM JIT (if available)")
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, ppuOptions)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        ppuDecoderSpinner.adapter = adapter

        // FPS options: 0->30, 1->45, 2->60, 3->120
        fpsSeek.max = 3
        fpsSeek.progress = 2
        fpsLabel.text = "Target FPS: 60"
        
        fpsSeek.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val fps = when (progress) {
                    0 -> 30
                    1 -> 45
                    2 -> 60
                    else -> 120
                }
                fpsLabel.text = "Target FPS: $fps"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        
        vsyncSwitch.isChecked = true
        rEdit.setText("0.03")
        gEdit.setText("0.03")
        bEdit.setText("0.08")

        applyBtn.setOnClickListener {
            val fps = when (fpsSeek.progress) {
                0 -> 30
                1 -> 45
                2 -> 60
                else -> 120
            }
            nativeSetTargetFps(fps)
            val r = rEdit.text.toString().toFloatOrNull() ?: 0.03f
            val g = gEdit.text.toString().toFloatOrNull() ?: 0.03f
            val b = bEdit.text.toString().toFloatOrNull() ?: 0.08f
            nativeSetClearColor(r, g, b)
            nativeSetVsync(vsyncSwitch.isChecked)
            
            Toast.makeText(this, "Settings saved", Toast.LENGTH_SHORT).show()
            finish()
        }
        
        cancelBtn.setOnClickListener {
            finish()
        }
    }
}
