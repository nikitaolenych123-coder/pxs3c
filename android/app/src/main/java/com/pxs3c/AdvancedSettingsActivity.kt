package com.pxs3c

import android.os.Bundle
import android.widget.*
import androidx.appcompat.widget.SwitchCompat

class AdvancedSettingsActivity : BaseActivity() {
    companion object {
        init {
            try {
                System.loadLibrary("pxs3c_jni")
                android.util.Log.i("PXS3C-JNI", "✓ Native library loaded in AdvancedSettings")
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("PXS3C-JNI", "✗ Failed to load pxs3c_jni in Advanced: ${e.message}", e)
            } catch (e: Exception) {
                android.util.Log.e("PXS3C-JNI", "✗ Unexpected error in Advanced: ${e.message}", e)
            }
        }
    }
    
    private external fun nativeSetSVE2Enabled(enabled: Boolean)
    private external fun nativeSetVulkanGPL(enabled: Boolean)
    private external fun nativeSetFSREnabled(enabled: Boolean)
    private external fun nativeSetThermalBypass(enabled: Boolean)
    private external fun nativeSetAsyncCompute(enabled: Boolean)
    private external fun nativeSetTargetFPS(fps: Int)
    
    private lateinit var sve2Switch: SwitchCompat
    private lateinit var gplSwitch: SwitchCompat
    private lateinit var fsrSwitch: SwitchCompat
    private lateinit var thermalBypassSwitch: SwitchCompat
    private lateinit var asyncComputeSwitch: SwitchCompat
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_advanced_settings)
        
        sve2Switch = findViewByIdSafe(R.id.switchSVE2) ?: return
        gplSwitch = findViewByIdSafe(R.id.switchGPL) ?: return
        fsrSwitch = findViewByIdSafe(R.id.switchFSR) ?: return
        thermalBypassSwitch = findViewByIdSafe(R.id.switchThermalBypass) ?: return
        asyncComputeSwitch = findViewByIdSafe(R.id.switchAsyncCompute) ?: return
        
        sve2Switch.setOnCheckedChangeListener { _, isChecked ->
            tryCatch("SVE2") {
                nativeSetSVE2Enabled(isChecked)
            }
        }
        
        gplSwitch.setOnCheckedChangeListener { _, isChecked ->
            tryCatch("Vulkan GPL") {
                nativeSetVulkanGPL(isChecked)
            }
        }
        
        fsrSwitch.setOnCheckedChangeListener { _, isChecked ->
            tryCatch("FSR") {
                nativeSetFSREnabled(isChecked)
            }
        }
        
        thermalBypassSwitch.setOnCheckedChangeListener { _, isChecked ->
            tryCatch("Thermal Bypass") {
                nativeSetThermalBypass(isChecked)
            }
        }
        
        asyncComputeSwitch.setOnCheckedChangeListener { _, isChecked ->
            tryCatch("Async Compute") {
                nativeSetAsyncCompute(isChecked)
            }
        }
    }
}
