package com.pxs3c

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import java.lang.Exception

/**
 * Base Activity with comprehensive error handling
 * All Activities should extend this instead of AppCompatActivity
 * Provides automatic exception catching and user-friendly error messages
 */
abstract class BaseActivity : AppCompatActivity() {
    
    companion object {
        const val TAG = "PXS3C-BaseActivity"
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        try {
            super.onCreate(savedInstanceState)
            Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
                handleUncaughtException(thread, throwable)
            }
        } catch (e: Exception) {
            handleException(e, "onCreate")
        }
    }
    
    override fun onStart() {
        try {
            super.onStart()
        } catch (e: Exception) {
            handleException(e, "onStart")
        }
    }
    
    override fun onResume() {
        try {
            super.onResume()
        } catch (e: Exception) {
            handleException(e, "onResume")
        }
    }
    
    override fun onPause() {
        try {
            super.onPause()
        } catch (e: Exception) {
            handleException(e, "onPause")
        }
    }
    
    override fun onStop() {
        try {
            super.onStop()
        } catch (e: Exception) {
            handleException(e, "onStop")
        }
    }
    
    override fun onDestroy() {
        try {
            super.onDestroy()
        } catch (e: Exception) {
            handleException(e, "onDestroy")
        }
    }
    
    /**
     * Safe findViewById with automatic type casting and error handling
     */
    protected inline fun <reified T : android.view.View> findViewByIdSafe(id: Int): T? {
        return try {
            val view = findViewById<android.view.View>(id)
            if (view is T) {
                Log.d(TAG, "✓ Found view ${T::class.simpleName} with id $id")
                view
            } else {
                Log.e(TAG, "✗ Type mismatch for view id $id: expected ${T::class.simpleName}, got ${view?.javaClass?.simpleName}")
                null
            }
        } catch (e: Exception) {
            Log.e(TAG, "✗ Error finding view id $id: ${e.message}", e)
            null
        }
    }
    
    /**
     * Central exception handler
     */
    protected fun handleException(exception: Exception, location: String) {
        val errorMsg = "Error in $location: ${exception.message}\n${exception.stackTraceToString()}"
        Log.e(TAG, "✗ $errorMsg", exception)
        
        // Show user-friendly toast
        showError("Application Error", "Error in $location:\n${exception.message}")
    }
    
    /**
     * Handle uncaught exceptions globally
     */
    private fun handleUncaughtException(thread: Thread, throwable: Throwable) {
        Log.e(TAG, "✗ UNCAUGHT EXCEPTION in thread ${thread.name}", throwable)
        
        runOnUiThread {
            showError(
                "Unexpected Error",
                "${throwable.javaClass.simpleName}: ${throwable.message}"
            )
        }
    }
    
    /**
     * Show error message to user with detailed logging
     */
    protected fun showError(title: String, message: String) {
        Log.e(TAG, "⚠️  $title: $message")
        
        try {
            runOnUiThread {
                try {
                    Toast.makeText(
                        this,
                        "$title: $message",
                        Toast.LENGTH_LONG
                    ).show()
                } catch (e: Exception) {
                    Log.e(TAG, "Could not show toast: ${e.message}")
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Could not runOnUiThread for toast: ${e.message}")
        }
    }
    
    /**
     * Safe execution wrapper
     */
    protected fun <T> tryCatch(
        action: String = "Operation",
        block: () -> T
    ): T? {
        return try {
            block()
        } catch (e: Exception) {
            Log.e(TAG, "✗ $action failed: ${e.message}", e)
            showError("Error", "$action failed:\n${e.message}")
            null
        }
    }
}
