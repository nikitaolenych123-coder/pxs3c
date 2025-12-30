package com.pxs3c

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.widget.*
import androidx.appcompat.widget.SwitchCompat

class AdvancedSettingsActivity : AppCompatActivity() {
    
    private lateinit var sve2Switch: SwitchCompat
    private lateinit var gplSwitch: SwitchCompat
    private lateinit var fsrSwitch: SwitchCompat
    private lateinit var thermalBypassSwitch: SwitchCompat
    private lateinit var asyncComputeSwitch: SwitchCompat
    private lateinit var frameRateSpinner: Spinner
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_advanced_settings)
        
        supportActionBar?.title = "Advanced Performance Settings"
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        
        initializeViews()
        loadSettings()
        setupListeners()
    }
    
    private fun initializeViews() {
        sve2Switch = findViewById(R.id.switchSVE2)
        gplSwitch = findViewById(R.id.switchGPL)
        fsrSwitch = findViewById(R.id.switchFSR)
        thermalBypassSwitch = findViewById(R.id.switchThermalBypass)
        asyncComputeSwitch = findViewById(R.id.switchAsyncCompute)
        frameRateSpinner = findViewById(R.id.spinnerFrameRate)
        
        // Setup frame rate spinner
        val frameRates = arrayOf("30 FPS", "60 FPS", "90 FPS (Experimental)", "120 FPS (Experimental)")
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, frameRates)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        frameRateSpinner.adapter = adapter
    }
    
    private fun loadSettings() {
        val prefs = getSharedPreferences("pxs3c_advanced", MODE_PRIVATE)
        
        sve2Switch.isChecked = prefs.getBoolean("sve2_acceleration", true)
        gplSwitch.isChecked = prefs.getBoolean("vulkan_gpl", true)
        fsrSwitch.isChecked = prefs.getBoolean("fsr_upscaling", false)
        thermalBypassSwitch.isChecked = prefs.getBoolean("thermal_bypass", false)
        asyncComputeSwitch.isChecked = prefs.getBoolean("async_compute", true)
        
        val targetFPS = prefs.getInt("target_fps", 60)
        frameRateSpinner.setSelection(when(targetFPS) {
            30 -> 0
            60 -> 1
            90 -> 2
            120 -> 3
            else -> 1
        })
    }
    
    private fun setupListeners() {
        findViewById<Button>(R.id.btnSaveAdvanced).setOnClickListener {
            saveSettings()
        }
        
        findViewById<Button>(R.id.btnResetAdvanced).setOnClickListener {
            resetToDefaults()
        }
        
        // Thermal bypass warning
        thermalBypassSwitch.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                Toast.makeText(
                    this,
                    "⚠️ Thermal Bypass is EXPERIMENTAL! May cause overheating.",
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }
    
    private fun saveSettings() {
        val prefs = getSharedPreferences("pxs3c_advanced", MODE_PRIVATE)
        val editor = prefs.edit()
        
        editor.putBoolean("sve2_acceleration", sve2Switch.isChecked)
        editor.putBoolean("vulkan_gpl", gplSwitch.isChecked)
        editor.putBoolean("fsr_upscaling", fsrSwitch.isChecked)
        editor.putBoolean("thermal_bypass", thermalBypassSwitch.isChecked)
        editor.putBoolean("async_compute", asyncComputeSwitch.isChecked)
        
        val targetFPS = when(frameRateSpinner.selectedItemPosition) {
            0 -> 30
            1 -> 60
            2 -> 90
            3 -> 120
            else -> 60
        }
        editor.putInt("target_fps", targetFPS)
        
        editor.apply()
        
        // Apply settings to native layer
        applyNativeSettings()
        
        Toast.makeText(this, "Advanced settings saved!", Toast.LENGTH_SHORT).show()
        finish()
    }
    
    private fun applyNativeSettings() {
        // Call native JNI methods to apply settings
        try {
            nativeSetSVE2Enabled(sve2Switch.isChecked)
            nativeSetVulkanGPL(gplSwitch.isChecked)
            nativeSetFSREnabled(fsrSwitch.isChecked)
            nativeSetThermalBypass(thermalBypassSwitch.isChecked)
            nativeSetAsyncCompute(asyncComputeSwitch.isChecked)
            
            val targetFPS = when(frameRateSpinner.selectedItemPosition) {
                0 -> 30
                1 -> 60
                2 -> 90
                3 -> 120
                else -> 60
            }
            nativeSetTargetFPS(targetFPS)
        } catch (e: Exception) {
            Toast.makeText(this, "Failed to apply native settings: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun resetToDefaults() {
        sve2Switch.isChecked = true
        gplSwitch.isChecked = true
        fsrSwitch.isChecked = false
        thermalBypassSwitch.isChecked = false
        asyncComputeSwitch.isChecked = true
        frameRateSpinner.setSelection(1) // 60 FPS
        
        Toast.makeText(this, "Reset to default settings", Toast.LENGTH_SHORT).show()
    }
    
    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }
    
    // Native methods
    private external fun nativeSetSVE2Enabled(enabled: Boolean)
    private external fun nativeSetVulkanGPL(enabled: Boolean)
    private external fun nativeSetFSREnabled(enabled: Boolean)
    private external fun nativeSetThermalBypass(enabled: Boolean)
    private external fun nativeSetAsyncCompute(enabled: Boolean)
    private external fun nativeSetTargetFPS(fps: Int)
}
