package com.spacexjr.jupiter.ui

import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.List
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Terminal
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.spacexjr.jupiter.R
import com.spacexjr.jupiter.ui.screens.ConfigScreen
import com.spacexjr.jupiter.ui.screens.EntriesScreen
import com.spacexjr.jupiter.ui.screens.FilesScreen
import com.spacexjr.jupiter.ui.screens.HomeScreen
import com.spacexjr.jupiter.ui.screens.LogsScreen
import com.spacexjr.jupiter.ui.screens.PatchScreen
import com.spacexjr.jupiter.ui.theme.JupiterTheme

sealed class Dest(val route: String, val labelRes: Int, val icon: ImageVector) {
    data object Home : Dest("home", R.string.nav_home, Icons.Filled.Home)
    data object Entries : Dest("entries", R.string.nav_entries, Icons.Filled.List)
    data object Logs : Dest("logs", R.string.nav_logs, Icons.Filled.Terminal)
    data object Settings : Dest("settings", R.string.nav_settings, Icons.Filled.Settings)
}

private val bottomDestinations = listOf(Dest.Home, Dest.Entries, Dest.Logs, Dest.Settings)

@Composable
fun JupiterRoot(vm: JupiterViewModel) {
    val navController = rememberNavController()
    val backStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = backStackEntry?.destination?.route
    val showBottomBar = bottomDestinations.any { it.route == currentRoute } ||
        currentRoute == null

    JupiterTheme(themeMode = vm.themeMode(), dynamicColor = vm.dynamicColor()) {
        Scaffold(
            bottomBar = {
                if (showBottomBar) {
                    NavigationBar {
                        bottomDestinations.forEach { dest ->
                            val selected = backStackEntry?.destination?.hierarchy
                                ?.any { it.route == dest.route } == true
                            NavigationBarItem(
                                selected = selected,
                                onClick = {
                                    if (!selected) {
                                        navController.navigate(dest.route) {
                                            popUpTo(Dest.Home.route) { saveState = true }
                                            launchSingleTop = true
                                            restoreState = true
                                        }
                                    }
                                },
                                icon = {
                                    Icon(
                                        dest.icon,
                                        contentDescription = stringResource(dest.labelRes),
                                    )
                                },
                                label = { Text(stringResource(dest.labelRes)) },
                            )
                        }
                    }
                }
            },
        ) { padding ->
            NavHost(
                navController = navController,
                startDestination = Dest.Home.route,
                modifier = Modifier.padding(padding),
            ) {
                composable(Dest.Home.route) {
                    HomeScreen(
                        vm = vm,
                        onPatch = { navController.navigate(PatchScreen.ROUTE) },
                        onFiles = { navController.navigate(FilesScreen.ROUTE) },
                    )
                }
                composable(Dest.Entries.route) { EntriesScreen(vm) }
                composable(Dest.Logs.route) { LogsScreen(vm) }
                composable(Dest.Settings.route) {
                    ConfigScreen(vm, onFiles = { navController.navigate(FilesScreen.ROUTE) })
                }
                composable(PatchScreen.ROUTE) {
                    PatchScreen(vm, onBack = { navController.popBackStack() })
                }
                composable(FilesScreen.ROUTE) {
                    FilesScreen(vm, onBack = { navController.popBackStack() })
                }
            }
        }
    }
}
