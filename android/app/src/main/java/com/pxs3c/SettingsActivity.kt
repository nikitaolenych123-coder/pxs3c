package com.pxs3c

import android.content.Context
import android.content.SharedPreferences
import android.net.Uri
import android.os.Bundle
import android.widget.*
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.SwitchCompat
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipInputStream

class SettingsActivity : AppCompatActivity() {
    companion object {
        init { System.loadLibrary("pxs3c_jni") }
    }

    external fun nativeSetTargetFps(fps: Int)
    external fun nativeSetClearColor(r: Float, g: Float, b: Float)
    external fun nativeSetVsync(enabled: Boolean)

    private lateinit var prefs: SharedPreferences
    private lateinit var firmwarePicker: ActivityResultLauncher<String>
    private lateinit var driverPicker: ActivityResultLauncher<String>
    private lateinit var firmwarePathText: TextView
    private lateinit var driverPathText: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        prefs = getSharedPreferences("pxs3c_settings", Context.MODE_PRIVATE)

        // Initialize pickers
        firmwarePicker = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            uri?.let { loadFirmware(it) }
        }
        
        driverPicker = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            uri?.let { loadCustomDriver(it) }
        }

        setupSystemSection()
        setupCpuSection()
        setupGpuSection()
        setupAudioSection()
        setupAdvancedSection()
        setupButtons()
        
        loadSettings()
    }

    private fun setupSystemSection() {
        val firmwareBtn = findViewById<Button>(R.id.btnLoadFirmware)
        firmwarePathText = findViewById(R.id.firmwarePathText)
        
        firmwareBtn.setOnClickListener {
            firmwarePicker.launch("application/zip")
        }
    }

    private fun setupCpuSection() {
        val ppuDecoderSpinner = findViewById<Spinner>(R.id.ppuDecoderSpinner)
        val spuDecoderSpinner = findViewById<Spinner>(R.id.spuDecoderSpinner)
        val threadCountSeek = findViewById<SeekBar>(R.id.threadCountSeek)
        val threadCountLabel = findViewById<TextView>(R.id.threadCountLabel)
        
        val ppuOptions = arrayOf("Interpreter (Precise)", "LLVM Recompiler (Fast)")
        val spuOptions = arrayOf("Interpreter (Precise)", "ASMJIT Recompiler (Fast)")
        
        ArrayAdapter(this, android.R.layout.simple_spinner_item, ppuOptions).also {
            it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            ppuDecoderSpinner.adapter = it
        }
        
        ArrayAdapter(this, android.R.layout.simple_spinner_item, spuOptions).also {
            it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            spuDecoderSpinner.adapter = it
        }
        
        threadCountSeek.max = 5 // 1-6 threads
        threadCountSeek.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                threadCountLabel.text = "SPU Threads: ${progress + 1}"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun setupGpuSection() {
        val fpsSeek = findViewById<SeekBar>(R.id.fpsSeek)
        val fpsLabel = findViewById<TextView>(R.id.fpsLabel)
        val vsyncSwitch = findViewById<SwitchCompat>(R.id.vsyncSwitch)
        val resolutionSpinner = findViewById<Spinner>(R.id.resolutionSpinner)
        val anisotropicSpinner = findViewById<Spinner>(R.id.anisotropicSpinner)
        val driverBtn = findViewById<Button>(R.id.btnLoadDriver)
        driverPathText = findViewById(R.id.driverPathText)
        
        val resOptions = arrayOf("Native (1x)", "2x (1440p)", "3x (2160p)", "4x (4K)")
        val anisoOptions = arrayOf("Auto", "Off", "2x", "4x", "8x", "16x")
        
        ArrayAdapter(this, android.R.layout.simple_spinner_item, resOptions).also {
            it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            resolutionSpinner.adapter = it
        }
        
        ArrayAdapter(this, android.R.layout.simple_spinner_item, anisoOptions).also {
            it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            anisotropicSpinner.adapter = it
        }
        
        fpsSeek.max = 3
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
        
        driverBtn.setOnClickListener {
            driverPicker.launch("application/zip")
        }
    }

    private fun setupAudioSection() {
        val audioBackendSpinner = findViewById<Spinner>(R.id.audioBackendSpinner)
        val audioLatencySeek = findViewById<SeekBar>(R.id.audioLatencySeek)
        val audioLatencyLabel = findViewById<TextView>(R.id.audioLatencyLabel)
        
        val backends = arrayOf("Cubeb (recommended)", "OpenSL ES", "Null (no audio)")
        ArrayAdapter(this, android.R.layout.simple_spinner_item, backends).also {
            it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            audioBackendSpinner.adapter = it
        }
        
        audioLatencySeek.max = 4 // 20ms - 100ms
        audioLatencySeek.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val latency = 20 + (progress * 20)
                audioLatencyLabel.text = "Audio Latency: ${latency}ms"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun setupAdvancedSection() {
        findViewById<SwitchCompat>(R.id.debugConsoleSwitch)
        findViewById<SwitchCompat>(R.id.accurateCacheSwitch)
        findViewById<SwitchCompat>(R.id.disableFrameSkipSwitch)
        val rEdit = findViewById<EditText>(R.id.editR)
        val gEdit = findViewById<EditText>(R.id.editG)
        val bEdit = findViewById<EditText>(R.id.editB)
        
        rEdit.setText("0.03")
        gEdit.setText("0.03")
        bEdit.setText("0.08")
    }

    private fun setupButtons() {
        val applyBtn = findViewById<Button>(R.id.applyBtn)
        val cancelBtn = findViewById<Button>(R.id.cancelBtn)
        val resetBtn = findViewById<Button>(R.id.resetBtn)

        applyBtn.setOnClickListener {
            saveSettings()
            Toast.makeText(this, "Settings saved", Toast.LENGTH_SHORT).show()
            finish()
        }
        
        cancelBtn.setOnClickListener {
            finish()
        }
        
        resetBtn.setOnClickListener {
            resetSettings()
            Toast.makeText(this, "Settings reset to defaults", Toast.LENGTH_SHORT).show()
        }
    }

    private fun saveSettings() {
        val editor = prefs.edit()
        
        // GPU
        val fps = when (findViewById<SeekBar>(R.id.fpsSeek).progress) {
            0 -> 30
            1 -> 45
            2 -> 60
            else -> 120
        }
        editor.putInt("target_fps", fps)
        editor.putBoolean("vsync", findViewById<SwitchCompat>(R.id.vsyncSwitch).isChecked)
        editor.putInt("resolution", findViewById<Spinner>(R.id.resolutionSpinner).selectedItemPosition)
        editor.putInt("anisotropic", findViewById<Spinner>(R.id.anisotropicSpinner).selectedItemPosition)
        
        // CPU
        editor.putInt("ppu_decoder", findViewById<Spinner>(R.id.ppuDecoderSpinner).selectedItemPosition)
        editor.putInt("spu_decoder", findViewById<Spinner>(R.id.spuDecoderSpinner).selectedItemPosition)
        editor.putInt("spu_threads", findViewById<SeekBar>(R.id.threadCountSeek).progress + 1)
        
        // Audio
        editor.putInt("audio_backend", findViewById<Spinner>(R.id.audioBackendSpinner).selectedItemPosition)
        editor.putInt("audio_latency", 20 + (findViewById<SeekBar>(R.id.audioLatencySeek).progress * 20))
        
        // Advanced
        editor.putBoolean("debug_console", findViewById<SwitchCompat>(R.id.debugConsoleSwitch).isChecked)
        editor.putBoolean("accurate_cache", findViewById<SwitchCompat>(R.id.accurateCacheSwitch).isChecked)
        editor.putBoolean("disable_frame_skip", findViewById<SwitchCompat>(R.id.disableFrameSkipSwitch).isChecked)
        
        val r = findViewById<EditText>(R.id.editR).text.toString().toFloatOrNull() ?: 0.03f
        val g = findViewById<EditText>(R.id.editG).text.toString().toFloatOrNull() ?: 0.03f
        val b = findViewById<EditText>(R.id.editB).text.toString().toFloatOrNull() ?: 0.08f
        editor.putFloat("clear_r", r)
        editor.putFloat("clear_g", g)
        editor.putFloat("clear_b", b)
        
        editor.apply()
        
        // Apply to native
        nativeSetTargetFps(fps)
        nativeSetClearColor(r, g, b)
        nativeSetVsync(findViewById<SwitchCompat>(R.id.vsyncSwitch).isChecked)
    }

    private fun loadSettings() {
        // GPU
        val fps = prefs.getInt("target_fps", 60)
        val fpsProgress = when (fps) {
            30 -> 0
            45 -> 1
            60 -> 2
            else -> 3
        }
        findViewById<SeekBar>(R.id.fpsSeek).progress = fpsProgress
        findViewById<SwitchCompat>(R.id.vsyncSwitch).isChecked = prefs.getBoolean("vsync", true)
        findViewById<Spinner>(R.id.resolutionSpinner).setSelection(prefs.getInt("resolution", 0))
        findViewById<Spinner>(R.id.anisotropicSpinner).setSelection(prefs.getInt("anisotropic", 0))
        
        // CPU
        findViewById<Spinner>(R.id.ppuDecoderSpinner).setSelection(prefs.getInt("ppu_decoder", 0))
        findViewById<Spinner>(R.id.spuDecoderSpinner).setSelection(prefs.getInt("spu_decoder", 0))
        findViewById<SeekBar>(R.id.threadCountSeek).progress = prefs.getInt("spu_threads", 6) - 1
        
        // Audio
        findViewById<Spinner>(R.id.audioBackendSpinner).setSelection(prefs.getInt("audio_backend", 0))
        val latency = prefs.getInt("audio_latency", 60)
        findViewById<SeekBar>(R.id.audioLatencySeek).progress = (latency - 20) / 20
        
        // Advanced
        findViewById<SwitchCompat>(R.id.debugConsoleSwitch).isChecked = prefs.getBoolean("debug_console", false)
        findViewById<SwitchCompat>(R.id.accurateCacheSwitch).isChecked = prefs.getBoolean("accurate_cache", true)
        findViewById<SwitchCompat>(R.id.disableFrameSkipSwitch).isChecked = prefs.getBoolean("disable_frame_skip", false)
        
        findViewById<EditText>(R.id.editR).setText(prefs.getFloat("clear_r", 0.03f).toString())
        findViewById<EditText>(R.id.editG).setText(prefs.getFloat("clear_g", 0.03f).toString())
        findViewById<EditText>(R.id.editB).setText(prefs.getFloat("clear_b", 0.08f).toString())
        
        // Paths
        firmwarePathText.text = prefs.getString("firmware_path", "Not installed") ?: "Not installed"
        driverPathText.text = prefs.getString("driver_path", "System default") ?: "System default"
    }

    private fun resetSettings() {
        prefs.edit().clear().apply()
        loadSettings()
    }

    private fun loadFirmware(uri: Uri) {
        try {
            val firmwareDir = File(filesDir, "firmware")
            firmwareDir.mkdirs()
            
            contentResolver.openInputStream(uri)?.use { input ->
                ZipInputStream(input).use { zip ->
                    var entry = zip.nextEntry
                    var fileCount = 0
                    while (entry != null) {
                        if (!entry.isDirectory) {
                            val file = File(firmwareDir, entry.name)
                            file.parentFile?.mkdirs()
                            FileOutputStream(file).use { output ->
                                zip.copyTo(output)
                            }
                            fileCount++
                        }
                        entry = zip.nextEntry
                    }
                    
                    prefs.edit().putString("firmware_path", firmwareDir.absolutePath).apply()
                    firmwarePathText.text = firmwareDir.absolutePath
                    Toast.makeText(this, "Firmware installed ($fileCount files)", Toast.LENGTH_SHORT).show()
                }
            }
        } catch (e: Exception) {
            Toast.makeText(this, "Failed to load firmware: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    private fun loadCustomDriver(uri: Uri) {
        try {
            val driverDir = File(filesDir, "custom_driver")
            driverDir.deleteRecursively()
            driverDir.mkdirs()
            
            contentResolver.openInputStream(uri)?.use { input ->
                ZipInputStream(input).use { zip ->
                    var entry = zip.nextEntry
                    while (entry != null) {
                        if (!entry.isDirectory) {
                            val file = File(driverDir, entry.name)
                            file.parentFile?.mkdirs()
                            FileOutputStream(file).use { output ->
                                zip.copyTo(output)
                            }
                        }
                        entry = zip.nextEntry
                    }
                }
            }
            
            prefs.edit().putString("driver_path", driverDir.absolutePath).apply()
            driverPathText.text = "Custom driver installed"
            Toast.makeText(this, "Custom driver installed. Restart required.", Toast.LENGTH_LONG).show()
        } catch (e: Exception) {
            Toast.makeText(this, "Failed to load driver: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }
}
