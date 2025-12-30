package com.pxs3c

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.*
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.io.File

class FilePickerActivity : AppCompatActivity() {
    private lateinit var fileListView: ListView
    private lateinit var currentPathText: TextView
    private lateinit var upButton: Button
    private lateinit var selectButton: Button
    private lateinit var cancelButton: Button
    
    private var currentDir = File("/sdcard")
    private var selectedFile: File? = null
    private var requestPermissionLauncher: ActivityResultLauncher<String>? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_file_picker)
        
        fileListView = findViewById(R.id.fileListView)
        currentPathText = findViewById(R.id.currentPathText)
        upButton = findViewById(R.id.upButton)
        selectButton = findViewById(R.id.selectButton)
        cancelButton = findViewById(R.id.cancelButton)
        
        // Setup permission launcher for Android 6+
        requestPermissionLauncher = registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted ->
            if (isGranted) {
                refreshFileList()
            } else {
                Toast.makeText(this, "Storage permission denied", Toast.LENGTH_SHORT).show()
            }
        }
        
        // Check storage permission
        checkAndRequestPermission()
        
        // Setup buttons
        upButton.setOnClickListener { goUp() }
        selectButton.setOnClickListener { selectFile() }
        cancelButton.setOnClickListener { cancel() }
        
        // Setup file list click listener
        fileListView.setOnItemClickListener { _, _, position, _ ->
            val adapter = fileListView.adapter as ArrayAdapter<*>
            val fileName = adapter.getItem(position).toString()
            val file = File(currentDir, fileName)
            
            if (file.isDirectory) {
                currentDir = file
                refreshFileList()
            } else {
                selectedFile = file
                selectButton.text = "Select: ${file.name}"
                selectButton.isEnabled = true
            }
        }
        
        refreshFileList()
    }
    
    private fun checkAndRequestPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            when {
                ContextCompat.checkSelfPermission(
                    this,
                    android.Manifest.permission.READ_EXTERNAL_STORAGE
                ) == PackageManager.PERMISSION_GRANTED -> {
                    refreshFileList()
                }
                else -> {
                    requestPermissionLauncher?.launch(
                        android.Manifest.permission.READ_EXTERNAL_STORAGE
                    )
                }
            }
        } else {
            refreshFileList()
        }
    }
    
    private fun refreshFileList() {
        val files = mutableListOf<String>()
        
        // Add parent directory option
        if (currentDir.parent != null) {
            files.add("..")
        }
        
        // List directories first
        currentDir.listFiles()?.forEach { file ->
            if (file.isDirectory && !file.name.startsWith(".")) {
                files.add(file.name + "/")
            }
        }
        
        // Then list PS3 game files (.self, .elf)
        currentDir.listFiles()?.forEach { file ->
            if (file.isFile) {
                val name = file.name.lowercase()
                if (name.endsWith(".self") || name.endsWith(".elf")) {
                    files.add(file.name)
                }
            }
        }
        
        // Update UI
        currentPathText.text = "Location: ${currentDir.absolutePath}"
        
        val adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, files)
        fileListView.adapter = adapter
        
        selectButton.isEnabled = false
        selectButton.text = "Select File"
        selectedFile = null
    }
    
    private fun goUp() {
        currentDir.parentFile?.let {
            currentDir = it
            refreshFileList()
        }
    }
    
    private fun selectFile() {
        selectedFile?.let { file ->
            val intent = Intent()
            intent.putExtra("FILE_PATH", file.absolutePath)
            setResult(RESULT_OK, intent)
            finish()
        } ?: run {
            Toast.makeText(this, "No file selected", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun cancel() {
        setResult(RESULT_CANCELED)
        finish()
    }
}
