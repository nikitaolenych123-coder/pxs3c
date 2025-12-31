package com.pxs3c

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.ParcelFileDescriptor
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.ListView
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream

class MainActivity : BaseActivity() {
    companion object {
        init {
            try {
                System.loadLibrary("pxs3c_jni")
                android.util.Log.i("PXS3C-JNI", "✓ Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("PXS3C-JNI", "✗ Failed to load pxs3c_jni: ${e.message}", e)
            } catch (e: Exception) {
                android.util.Log.e("PXS3C-JNI", "✗ Unexpected error loading native library: ${e.message}", e)
            }
        }
    }

    external fun nativeInit(): Boolean
    external fun nativeLoadGame(path: String): Boolean
    external fun nativeRunFrame()
    external fun nativeShutdown()
    external fun nativeAttachSurface(surface: android.view.Surface): Boolean
    external fun nativeTickFrame(): Int
    external fun nativeGetStatus(): String
    external fun nativeResize(width: Int, height: Int): Boolean
    external fun nativeSetClearColor(r: Float, g: Float, b: Float)
    external fun nativeSetVsync(enabled: Boolean)
    
    private lateinit var filePickerLauncher: ActivityResultLauncher<Array<String>>
    private var gameLoaded = false
    private var isRunning = false
    private lateinit var surfaceView: SurfaceView
    private lateinit var statusText: TextView
    private lateinit var fpsText: TextView
    private lateinit var gameListView: ListView
    private var gameAdapter: android.widget.ArrayAdapter<String>? = null
    private var gameUris: MutableList<String> = mutableListOf()
    private var gameTypes: MutableList<String> = mutableListOf()
    private var btnStop: android.view.View? = null
    private var btnBootGame: android.view.View? = null

    private val libraryPrefs by lazy { getSharedPreferences("pxs3c_library", MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        try {
            super.onCreate(savedInstanceState)
            setContentView(R.layout.activity_main)
            
            // Use safe findViewById with automatic type handling
            surfaceView = findViewByIdSafe(R.id.surfaceView) ?: return
            statusText = findViewByIdSafe(R.id.statusText) ?: return
            fpsText = findViewByIdSafe(R.id.fpsText) ?: return
            gameListView = findViewByIdSafe(R.id.gameListView) ?: return
            val btnSettings: android.view.View? = findViewByIdSafe(R.id.btnSettings)
            val btnLoadGame: android.view.View? = findViewByIdSafe(R.id.btnLoadGame)
            btnBootGame = findViewByIdSafe(R.id.btnBootGame)
            btnStop = findViewByIdSafe(R.id.btnStop)
            val btnRefresh: android.view.View? = findViewByIdSafe(R.id.btnRefresh)
            
            statusText.text = "✓ UI Initialized"
            android.util.Log.i("PXS3C-Main", "✓ onCreate started successfully")

            // Apply overlay visibility early (before surface callbacks)
            try {
                val prefs = getSharedPreferences("pxs3c_settings", MODE_PRIVATE)
                val showFps = prefs.getBoolean("show_fps", true)
                fpsText.visibility = if (showFps) android.view.View.VISIBLE else android.view.View.GONE
            } catch (_: Exception) {
                // Ignore
            }
        
            // Use SAF (Storage Access Framework) for picking executables
            filePickerLauncher = registerForActivityResult(
                ActivityResultContracts.OpenDocument()
            ) { uri: Uri? ->
                uri?.let { addGameToLibrary(it) }
            }

            gameAdapter = android.widget.ArrayAdapter(this, R.layout.item_game, R.id.gameTitle, mutableListOf<String>())
            gameListView.adapter = gameAdapter
            gameListView.setOnItemClickListener { _, _, position, _ ->
                val uriString = gameUris.getOrNull(position) ?: return@setOnItemClickListener
                val type = gameTypes.getOrNull(position) ?: "exec"
                val uri = Uri.parse(uriString)
                if (type == "iso") {
                    Toast.makeText(this, "ISO added. Booting ISO is not supported yet.", Toast.LENGTH_LONG).show()
                    return@setOnItemClickListener
                }
                bootGameFromLibrary(uri)
            }

            refreshGameLibrary()

            surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
                override fun surfaceCreated(holder: SurfaceHolder) {
                    try {
                        // Delayed initialization - only attach surface, init happens in nativeAttachSurface
                        if (!nativeAttachSurface(holder.surface)) {
                            runOnUiThread {
                                statusText.text = "Surface initialization pending..."
                                android.util.Log.w("PXS3C-Main", "Surface attach returned false, may retry later")
                            }
                            return
                        }
                        
                        try {
                            val prefs = getSharedPreferences("pxs3c_settings", MODE_PRIVATE)
                            val vsync = prefs.getBoolean("vsync", true)
                            val showFps = prefs.getBoolean("show_fps", true)
                            val r = prefs.getFloat("clear_r", 0.03f)
                            val g = prefs.getFloat("clear_g", 0.03f)
                            val b = prefs.getFloat("clear_b", 0.08f)

                            nativeSetClearColor(r, g, b)
                            nativeSetVsync(vsync)

                            runOnUiThread {
                                fpsText.visibility = if (showFps) android.view.View.VISIBLE else android.view.View.GONE
                            }
                        } catch (e: Exception) {
                            android.util.Log.w("PXS3C-Main", "Failed to apply saved settings: ${e.message}")
                        }
                        
                        runOnUiThread {
                            statusText.text = "Ready - Load a game to start"
                        }
                    } catch (e: Exception) {
                        runOnUiThread {
                            statusText.text = "Startup OK - Surface pending"
                            android.util.Log.w("PXS3C-Main", "Surface init non-critical error: ${e.message}")
                        }
                    }
                }
                
                override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                    try {
                        nativeResize(width, height)
                    } catch (e: Exception) {
                        // Ignore resize errors
                    }
                }
                
                override fun surfaceDestroyed(holder: SurfaceHolder) {
                    try {
                        stopFrameLoop()
                        nativeShutdown()
                    } catch (e: Exception) {
                        // Ignore shutdown errors
                    }
                }
            })

            btnSettings?.setOnClickListener {
                startActivity(Intent(this, SettingsActivity::class.java))
            }
            
            btnLoadGame?.setOnClickListener {
                statusText.text = "Add game: select EBOOT.BIN/.self/.elf (ISO can be added, boot later)"
                filePickerLauncher.launch(arrayOf("*/*"))
            }
            
            btnBootGame?.setOnClickListener {
                if (!gameLoaded) {
                    Toast.makeText(this, "Please load a game first", Toast.LENGTH_SHORT).show()
                    return@setOnClickListener
                }
                if (!isRunning) {
                    startFrameLoop()
                    btnStop?.isEnabled = true
                    (it as? android.widget.Button)?.text = "Pause"
                } else {
                    stopFrameLoop()
                    (it as? android.widget.Button)?.text = "Resume"
                }
            }
            
            btnStop?.setOnClickListener {
                stopFrameLoop()
                gameLoaded = false
                btnStop?.isEnabled = false
                (btnBootGame as? android.widget.Button)?.text = "Boot Game"
                statusText.text = "Stopped"
                fpsText.text = "FPS: 0"
            }
            
            btnRefresh?.setOnClickListener {
                refreshGameLibrary()
                Toast.makeText(this, "Game library refreshed", Toast.LENGTH_SHORT).show()
            }
        } catch (e: Exception) {
            handleException(e, "MainActivity.onCreate")
        }
    }

    private fun refreshGameLibrary() {
        val raw = libraryPrefs.getString("games", "[]") ?: "[]"
        val titles = mutableListOf<String>()
        val uris = mutableListOf<String>()
        val types = mutableListOf<String>()

        try {
            val arr = JSONArray(raw)
            for (i in 0 until arr.length()) {
                val obj = arr.optJSONObject(i) ?: continue
                val title = obj.optString("title")
                val uri = obj.optString("uri")
                val type = obj.optString("type")
                if (title.isNotBlank() && uri.isNotBlank()) {
                    val inferredType = when {
                        type.isNotBlank() -> type
                        title.lowercase().endsWith(".iso") -> "iso"
                        else -> "exec"
                    }

                    titles.add(
                        when (inferredType) {
                            "iso" -> "$title (ISO)"
                            else -> title
                        }
                    )
                    uris.add(uri)
                    types.add(inferredType)
                }
            }
        } catch (_: Exception) {
            // If corrupted, reset
        }

        gameUris = uris
        gameTypes = types
        gameAdapter?.clear()
        gameAdapter?.addAll(titles)
        gameAdapter?.notifyDataSetChanged()
    }

    private fun addGameToLibrary(uri: Uri) {
        try {
            // Persist permission so we can open the file later
            try {
                contentResolver.takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
                )
            } catch (_: Exception) {
                // Some providers don’t support persistable perms; we still try to copy when booting.
            }

            val fileName = getFileName(uri) ?: "game"
            val lowerName = fileName.lowercase()

            val isSupportedExecutable = lowerName.endsWith(".elf") ||
                lowerName.endsWith(".self") ||
                lowerName == "eboot.bin" ||
                lowerName == "boot.bin"

            val isIso = lowerName.endsWith(".iso")

            if (lowerName.endsWith(".pkg")) {
                runOnUiThread {
                    statusText.text = "Not supported yet: $fileName"
                    Toast.makeText(this, "PKG not supported yet. Add EBOOT.BIN/.self/.elf or ISO.", Toast.LENGTH_LONG).show()
                }
                return
            }

            if (!isSupportedExecutable && !isIso) {
                runOnUiThread {
                    statusText.text = "Unsupported file: $fileName"
                    Toast.makeText(
                        this,
                        "Unsupported file. Select EBOOT.BIN/.self/.elf or an .iso image.",
                        Toast.LENGTH_LONG
                    ).show()
                }
                return
            }

            val type = if (isIso) "iso" else "exec"

            val entry = JSONObject().apply {
                put("title", fileName)
                put("uri", uri.toString())
                put("type", type)
            }

            val raw = libraryPrefs.getString("games", "[]") ?: "[]"
            val arr = try { JSONArray(raw) } catch (_: Exception) { JSONArray() }

            // De-dupe by uri
            for (i in 0 until arr.length()) {
                val obj = arr.optJSONObject(i) ?: continue
                if (obj.optString("uri") == uri.toString()) {
                    runOnUiThread {
                        statusText.text = "Already added: $fileName"
                        Toast.makeText(this, "Game already in library", Toast.LENGTH_SHORT).show()
                    }
                    return
                }
            }

            arr.put(entry)
            libraryPrefs.edit().putString("games", arr.toString()).apply()

            runOnUiThread {
                statusText.text = "Added: $fileName"
                refreshGameLibrary()
            }
        } catch (e: Exception) {
            runOnUiThread {
                statusText.text = "Add failed: ${e.message}"
                Toast.makeText(this, "Add failed: ${e.message}", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun bootGameFromLibrary(uri: Uri) {
        if (isRunning) {
            Toast.makeText(this, "Stop emulation first", Toast.LENGTH_SHORT).show()
            return
        }
        statusText.text = "Booting selected game..."
        loadGameFromUri(uri)
    }
    
    private fun loadGameFromUri(uri: Uri) {
        Thread {
            try {
                val fileName = getFileName(uri) ?: "game.tmp"
                val lowerName = fileName.lowercase()

                val isSupportedExecutable = lowerName.endsWith(".elf") ||
                    lowerName.endsWith(".self") ||
                    lowerName == "eboot.bin" ||
                    lowerName == "boot.bin"
                val isIso = lowerName.endsWith(".iso")

                // Current core loader supports ELF/SELF only.
                // PKG/ISO require additional install/mount + decryption pipeline.
                if (lowerName.endsWith(".pkg")) {
                    runOnUiThread {
                        statusText.text = "Format not supported yet: $fileName"
                        Toast.makeText(
                            this,
                            "PKG not supported yet. Select EBOOT.BIN/.self/.elf.",
                            Toast.LENGTH_LONG
                        ).show()
                    }
                    return@Thread
                }

                if (isIso) {
                    runOnUiThread {
                        statusText.text = "ISO added (boot not supported yet): $fileName"
                        Toast.makeText(
                            this,
                            "ISO is added to library, but booting ISO is not supported yet.",
                            Toast.LENGTH_LONG
                        ).show()
                    }
                    return@Thread
                }

                if (!isSupportedExecutable) {
                    runOnUiThread {
                        statusText.text = "Unsupported file: $fileName"
                        Toast.makeText(
                            this,
                            "Unsupported file. Select EBOOT.BIN/.self/.elf.",
                            Toast.LENGTH_LONG
                        ).show()
                    }
                    return@Thread
                }

                val tempFile = File(cacheDir, fileName)
                
                runOnUiThread {
                    statusText.text = "Loading: $fileName..."
                }
                
                // Buffered copy for performance
                val inputStream = contentResolver.openInputStream(uri)
                if (inputStream == null) {
                    runOnUiThread {
                        statusText.text = "Can’t open file (permission/provider issue)"
                        Toast.makeText(this, "Can’t open the selected file. Try re-adding it.", Toast.LENGTH_LONG).show()
                    }
                    return@Thread
                }

                inputStream.use { input ->
                    FileOutputStream(tempFile).buffered(16384).use { output ->
                        input.copyTo(output, 16384)
                    }
                }
                
                val success = nativeLoadGame(tempFile.absolutePath)
                
                runOnUiThread {
                    if (success) {
                        gameLoaded = true
                        statusText.text = "Game loaded: $fileName"
                        Toast.makeText(this, "First boot may compile PPU/SPU caches. Watch logcat.", Toast.LENGTH_LONG).show()
                        
                        // Start frame loop after successful game load
                        startFrameLoop()
                    } else {
                        statusText.text = "Failed to load game - Check logcat for details"
                        Toast.makeText(this, "Failed to load game", Toast.LENGTH_SHORT).show()
                        tempFile.delete() // Cleanup on failure
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    statusText.text = "Error: ${e.message}"
                    Toast.makeText(this, "Error loading file: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }.start()
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
    
    private var frameHandler: android.os.Handler? = null
    private var frameRunnable: Runnable? = null
    private var fpsFrames: Int = 0
    private var fpsWindowStartNs: Long = 0L
    private var lastFpsValue: Int = 0
    private var lastStatusText: String = ""
    private var lastStatusUpdateMs: Long = 0L
    
    private fun startFrameLoop() {
        if (isRunning) return
        isRunning = true
        statusText.text = "Emulator running"

        fpsFrames = 0
        fpsWindowStartNs = System.nanoTime()
        lastFpsValue = 0
        lastStatusText = ""
        lastStatusUpdateMs = 0L
        
        frameHandler = android.os.Handler(android.os.Looper.getMainLooper())
        frameRunnable = object : Runnable {
            override fun run() {
                if (!isRunning) return
                try {
                    // Pull a short status string from native core.
                    // Avoid updating TextView every tick (can cause jank).
                    try {
                        val nowMs = android.os.SystemClock.uptimeMillis()
                        if (nowMs - lastStatusUpdateMs >= 250L) {
                            val s = nativeGetStatus()
                            val trimmed = s.trim()
                            if (trimmed.isNotEmpty() && trimmed != lastStatusText) {
                                lastStatusText = trimmed
                                statusText.text = trimmed
                            }
                            lastStatusUpdateMs = nowMs
                        }
                    } catch (_: Exception) {
                        // Ignore status errors
                    }

                    // Simple FPS counter based on UI loop cadence (good enough for an overlay).
                    fpsFrames += 1
                    val nowNs = System.nanoTime()
                    val elapsedNs = nowNs - fpsWindowStartNs
                    if (elapsedNs >= 1_000_000_000L) {
                        val fps = (fpsFrames * 1_000_000_000L / elapsedNs).toInt().coerceAtLeast(0)
                        fpsFrames = 0
                        fpsWindowStartNs = nowNs
                        if (fps != lastFpsValue) {
                            lastFpsValue = fps
                            fpsText.text = "FPS: $fps"
                        }
                    }

                    val delay = nativeTickFrame().toLong().coerceIn(1L, 100L)
                    frameHandler?.postDelayed(this, delay)
                } catch (e: Exception) {
                    isRunning = false
                    runOnUiThread {
                        statusText.text = "Error: ${e.message}"
                    }
                }
            }
        }
        frameHandler?.post(frameRunnable!!)
    }
    
    private fun stopFrameLoop() {
        isRunning = false
        frameRunnable?.let { frameHandler?.removeCallbacks(it) }
        frameHandler = null
        frameRunnable = null
    }

    override fun onDestroy() {
        try {
            stopFrameLoop()
            nativeShutdown()
            // Cleanup cache
            cacheDir.listFiles()?.forEach { 
                try {
                    it.delete()
                } catch (e: Exception) {
                    // Ignore delete errors
                }
            }
        } catch (e: Exception) {
            // Ignore cleanup errors
        }
        super.onDestroy()
    }
    
    override fun onPause() {
        super.onPause()
        stopFrameLoop()
    }
    
    override fun onResume() {
        super.onResume()
        if (gameLoaded && !isRunning) {
            startFrameLoop()
        }
    }
}
