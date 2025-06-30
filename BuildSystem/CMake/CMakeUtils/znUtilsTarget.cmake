function(zenith_enable_lto target)
	message(STATUS "Applying optimizations to: ${target}")

	if(IPO_SUPPORTED)
		set_target_properties(${target} PROPERTIES
				INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
				INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
		)
		message(STATUS "  -> IPO enabled for Release builds")
	endif()
endfunction()

function(zenith_copy_directory target source_dir dest_subdir)
	add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/${source_dir}
			$<TARGET_FILE_DIR:${target}>/${dest_subdir}
			COMMENT "Copying ${source_dir} to ${target} at ${dest_subdir}"
			VERBATIM
	)
endfunction()

function(zenith_copy_directory_absolute target source_dir dest_subdir)
	add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${source_dir}"
			"$<TARGET_FILE_DIR:${target}>/${dest_subdir}"
			COMMENT "Copying ${source_dir} to ${target} at ${dest_subdir}"
			VERBATIM
	)
endfunction()

function(zenith_add_sanitizers target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /fsanitize=address)
	else()
		target_compile_options(${target} PRIVATE
				-fsanitize=address
				-fno-omit-frame-pointer
				-g
		)
		target_link_options(${target} PRIVATE -fsanitize=address)
	endif()
endfunction()

function(zenith_apply_coverage target)
	if(ZENITH_ENABLE_COVERAGE AND ZENITH_COVERAGE_COMPILE_FLAGS)
		target_compile_options(${target} PRIVATE ${ZENITH_COVERAGE_COMPILE_FLAGS})
		target_link_options(${target} PRIVATE ${ZENITH_COVERAGE_LINK_FLAGS})
		message(STATUS "Applied coverage to: ${target}")
	endif()
endfunction()