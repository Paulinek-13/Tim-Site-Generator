// ==================================================
// file: main.cpp
// project: Tim Site Generator
// author: Paulina Kalicka
// ==================================================

constexpr const char* TIM_VERSION = "v1.0";
constexpr const char* TIM_NAME    = "Tim Site Generator";
constexpr const char* TIM_AUTHOR  = "Paulina Kalicka";

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

std::error_code error_code; // error code from filesystem

#define wait        \
	std::cin.get(); \
	std::cin.get()

#define log_word(x)    (std::cout << x)
#define log_line(x)    (std::cout << x << "\n")
#define log_success(x) (std::cout << "++++++++++ " << x << " ++++++++++" \
	                              << "\n")
#define log_failure(x) (std::cout << "---------- " << x << " ----------" \
	                              << "\n")
#define log_error_code                  \
	if(error_code != std::error_code()) \
	std::cout << "Error code: " << error_code << " " << error_code.message() << "\n"

#define COMMAND_STRUCTURE   ("tim [site_task] [site_name]   /   tim [app_task]")
#define POSSIBLE_SITE_TASKS ("new / build / clean / info / delete / pack")
#define POSSIBLE_APP_TASKS  ("help / v / todo")

#define EXAMPLE_FOLDER "data\\example"

struct Site
{
	std::string name;

	std::string directory;
	std::string base_file_dir;
	std::string feed_dir;
	std::string index_file_dir;
	std::string output_dir;
	std::string data_file_dir;
	std::string config_file_dir;

	std::unordered_map<std::string, std::string> data;
	std::unordered_map<std::string, std::string> config;
};

/////////////////////////////////////////////////////
// < HELPFUL FUNCTIONS

/**/
static std::string GetWithoutHTMLExtension(std::string filename)
{
	std::string name = filename;
	name.erase(name.end() - 5, name.end());
	return name;
}

/* split string with given delimer and return vector with string values */
static std::vector<std::string> SplitString(std::string str, char del)
{
	std::vector<std::string> vec;
	std::string              word;
	unsigned                 size = str.size();
	char                     ch;

	for(unsigned i = 0; i < size; ++i)
	{
		ch = str.at(i);
		if(ch == del)
		{
			vec.push_back(word);
			word = "";
		}
		else if((i == size - 1))
		{
			word += ch;
			vec.push_back(word);
			word = "";
		}
		else
			word += ch;
	}
	return vec;
}

/* get a page's title: 
 if file is an index file title is the parent directory name
 otherwise title is just file name witout extension */
static std::string GetCurrentTitle(std::string file_dir)
{
	std::filesystem::path path = std::filesystem::path(file_dir);
	if(path.filename().string() == "index.html")
		return path.parent_path().filename().string();
	else
		return GetWithoutHTMLExtension(path.filename().string());
}

/* get current file full final URL */
static std::string GetCurrentURL(std::string baseURL, std::string curr_dir)
{
	std::string rel_dir = std::filesystem::relative(curr_dir, baseURL, error_code).parent_path().string();
	log_error_code;
	return baseURL + "\\" + rel_dir;
}

/* initialize site's essential directories */
static void InitDirs(Site* site)
{
	site->directory = std::filesystem::current_path(error_code).string() + '\\' + site->name;
	log_error_code;
	log_line("Directory: " << site->directory);

	site->base_file_dir = site->directory + '\\' + "_base.html";
	log_line("Base file directory: " << site->base_file_dir);

	site->feed_dir = site->directory + '\\' + "_feed";
	log_line("Feed directory: " << site->feed_dir);

	site->index_file_dir = site->feed_dir + '\\' + "index.html";
	log_line("Main index.html file directory: " << site->index_file_dir);

	site->output_dir = site->directory + '\\' + site->name;
	log_line("Output directory: " << site->output_dir);

	site->data_file_dir = site->directory + '\\' + "_data.txt";
	log_line("Data file directory: " << site->data_file_dir);

	site->config_file_dir = site->directory + '\\' + "_config.txt";
	log_line("Config file directory: " << site->config_file_dir);
}

/* write links to all pages that are */
static void WritePrevLinks(std::string curr_dir, Site* site, std::ofstream& output)
{
	std::string rel_dir = std::filesystem::relative(curr_dir, site->output_dir, error_code).string();
	log_error_code;
	std::string link_name = "";
	std::string rel_link  = "";
	bool        added     = false;
	for(char ch : rel_dir)
	{
		if(ch == '\\')
		{
			if(!added)
			{
				output << "<nav class=\"nav-links prev-links\" >";
				added = true;
			}
			output << "<a class=\"nav-link prev-link\" href=\"" + site->data["url"] + "/" + rel_link + "\">" + link_name + "</a>";

			link_name.clear();
		}
		else
			link_name += ch;
		rel_link += ch;
	}
	if(added)
		output << "</nav>";
}

/* write links to all next folders (not files) */
static void WriteNextLinks(std::string curr_dir, Site* site, std::ofstream& output)
{
	curr_dir   = std::filesystem::path(curr_dir).parent_path().string();
	bool added = false;
	for(const auto& entry : std::filesystem::directory_iterator(curr_dir, error_code))
	{
		log_error_code;
		if(entry.is_directory(error_code))
		{
			log_error_code;
			if(!added)
			{
				output << "<nav class=\"nav-links next-links\" >";
				added = true;
			}
			std::string name = entry.path().filename().string();
			output << "<a class=\"nav-link next-link\" href=\"" + name + "\">" + name + "</a>";
		}
	}
	if(added)
		output << "</nav>";
}

/* write links to all folders/pages that are just after _feed directory */
static void WriteFeedLinks(Site* site, std::ofstream& output)
{
	output << "<nav class=\"nav-links feed-links\" >";
	output << "<a class=\"nav-link feed-link site-link\" href=\"" + site->data["url"] + "\">" + site->name + "</a>";
	for(const auto& entry : std::filesystem::directory_iterator(site->output_dir, error_code))
	{
		log_error_code;
		if(entry.is_directory(error_code))
		{
			log_error_code;
			std::string name = entry.path().filename().string();
			output << "<a class=\"nav-link feed-link\" href=\"" + site->data["url"] + "/" + name + "\">" + name + "</a>";
		}
	}
	output << "</nav>";
}

/* write links to all HTML files in the current directory */
static void WritePageLinks(std::string curr_dir, Site* site, std::ofstream& output)
{
	bool write_index = (site->config["index_page"] == "false") ? (false) : (true);
	curr_dir         = std::filesystem::path(curr_dir).parent_path().string();
	bool added       = false;
	for(const auto& entry : std::filesystem::directory_iterator(curr_dir, error_code))
	{
		log_error_code;
		if(entry.path().extension().string() == ".html")
		{
			std::string filename = entry.path().filename().string();
			if((!write_index) && (filename == "index.html")) continue;
			if(!added)
			{
				output << "<nav class=\"nav-links page-links\" >";
				added = true;
			}
			output << "<a class=\"nav-link page-link\" href=\"" + filename + "\">" + GetWithoutHTMLExtension(filename) + "</a>";
		}
	}
	if(added)
		output << "</nav>";
}

/* check if all needed folder and files are in right places */
static bool CheckForDirsAndFiles(Site* site)
{
	bool result = true;
	auto is_dir = [&](std::string dir) {
		if(!std::filesystem::is_directory(dir, error_code))
		{
			log_failure(dir << " is NOT a valid directory");
			log_error_code;
			result = false;
		}
	};
	auto is_file = [&](std::string file_dir) {
		if(!std::filesystem::is_regular_file(file_dir, error_code))
		{
			log_failure(file_dir << " is NOT a valid file directory");
			log_error_code;
			result = false;
		}
	};

	is_dir(site->directory);
	is_file(site->base_file_dir);
	is_dir(site->feed_dir);
	is_file(site->index_file_dir);
	//is_dir(site->output_dir);
	is_file(site->data_file_dir);

	return result;
}

/* read data/config site file (_data.txt/_config.txt)
 and save keys and values in site's unordered map */
static bool ReadDataFile(std::string file_dir, std::unordered_map<std::string, std::string>& umap)
{
	std::ifstream file(file_dir);
	if(file.is_open())
	{
		std::string key;
		std::string val;
		while(file.peek() != EOF)
		{
			std::getline(file, key, ':');
			std::getline(file, val);
			umap[key] = val;
		}
		file.close();
	}
	else
	{
		log_failure(file_dir << " file is NOT open");
		return false;
	}

	return true;
}

/* read data at the beginning of a file
 and save keys and values to given unordered map */
static void ReadCurrFileData(std::ifstream& file, std::unordered_map<std::string, std::string>& data)
{
	std::string key;
	std::string val;
	while(file.peek() != EOF)
	{
		std::getline(file, key, ':');
		if(key == ";")
			return;
		std::getline(file, val);
		data[key] = val;
	}
}

/* generate a single final HTML file */
static bool GenerateHTMLFile(std::ifstream& base, std::string output_dir, Site* site, std::string content_dir)
{
	bool result      = true;
	bool write_value = true;

	std::ofstream output(output_dir);
	if(output.is_open())
	{
		std::unordered_map<std::string, std::string> data;

		std::ifstream content(content_dir);
		if(content.is_open())
			ReadCurrFileData(content, data);
		else
		{
			log_failure("Content file: " << content_dir << " is NOT open");
			return false;
		}

		std::function<bool(std::ifstream&)>
		    write = [&](std::ifstream& base) -> bool {
			std::string str_line;
			char        ch;
			while(base.peek() != EOF)
			{
				std::getline(base, str_line, '~');
				if(write_value)
					output << str_line;
				if(base.eof())
					break;
				base >> ch; // take a chracter that indicates type of data
				std::string str_data_to_replace;
				std::getline(base, str_data_to_replace, '~'); // take a name and optionally arguments
				std::vector<std::string> arguments = SplitString(str_data_to_replace, '.');
				unsigned                 args_size = arguments.size();
				str_data_to_replace                = (args_size) ? (arguments[0]) : ("");
				if(str_data_to_replace == "endif") write_value = true;
				else if(write_value)
				{
					switch(ch)
					{
						// already defined data
						case '_':
						{
							if(str_data_to_replace == "content")
							{
								if(!write(content))
								{
									log_failure("Page content was NOT written properly");
									result = false;
								}
							}
							else if(str_data_to_replace == "name")
								output << site->name;
							else if(str_data_to_replace == "url")
								output << site->data["url"];
							else if(str_data_to_replace == "this_url")
								output << GetCurrentURL(site->data["url"], output_dir);
							else if(str_data_to_replace == "title")
								output << GetCurrentTitle(output_dir);
							else if(str_data_to_replace == "prev_links")
								WritePrevLinks(output_dir, site, output);
							else if(str_data_to_replace == "next_links")
								WriteNextLinks(output_dir, site, output);
							else if(str_data_to_replace == "feed_links")
								WriteFeedLinks(site, output);
							else if(str_data_to_replace == "page_links")
								WritePageLinks(output_dir, site, output);
							else if((str_data_to_replace == "if") || (str_data_to_replace == "ifnot"))
							{
								bool neg    = (str_data_to_replace == "if") ? (false) : (true);
								write_value = neg;
								if(args_size >= 2)
								{
									if((arguments[1] == "page") && (args_size == 3))
									{
										if((arguments[2] == "site_index") && (content_dir == site->index_file_dir)) write_value = !neg;
									}
								}
							}
							else
								log_failure("Undefined data to replace: " << str_data_to_replace);
						}
						break;

						//// a file from current folder
						//case '=':
						//{
						//	std::ifstream file_include(output_dir + "\\" + str_data_to_replace);
						//	if(file_include.is_open())
						//	{
						//		log_success("A file to include is open");
						//		std::string line;
						//		while(file_include.peek() != EOF)
						//		{
						//			std::getline(file_include, line, '\n');
						//			output << line << '\n';
						//		}
						//		log_success("A file to include was included");
						//		file_include.close();
						//	}
						//	else
						//		log_failure("A file to include is NOT open hence CANNOT be included");
						//}
						//break;

						// a data defined in _data.txt
						case ':':
						{
							if(site->data[str_data_to_replace].size())
								output << site->data[str_data_to_replace];
							else
								log_failure("Undefined data to replace: " << str_data_to_replace);
						}
						break;

						// a data defined in current content file
						case '+':
						{
							if(data[str_data_to_replace].size())
								output << data[str_data_to_replace];
							else
								log_failure("Undefined data to replace: " << str_data_to_replace);
						}
						break;

						default: log_failure("Undefined token: " << ch); break;
					}
				}
			}
			return true;
		};

		if(!write(base))
		{
			log_failure("Page was NOT written properly");
			result = false;
		}

		content.close();
		output.close();
	}
	else
	{
		log_failure("File: " << output_dir << " is NOT open");
		return false;
	}

	return result;
}

/* generate HTML final site files in output folder */
static bool GenerateFiles(Site* site)
{
	std::string name, output_dir, content_dir;

	for(const auto& feed_entry : std::filesystem::recursive_directory_iterator(site->output_dir, error_code))
	{
		log_error_code;
		name        = feed_entry.path().filename().string();
		output_dir  = feed_entry.path().string();
		content_dir = site->feed_dir + "\\" + std::filesystem::relative(output_dir, site->output_dir, error_code).string();
		log_error_code;

		if(feed_entry.is_regular_file(error_code))
		{
			if(feed_entry.path().extension().string() == ".html")
			{
				std::ifstream base_html(site->base_file_dir);
				if(base_html.is_open())
				{
					if(!GenerateHTMLFile(base_html, output_dir, site, content_dir))
					{
						log_failure("File: " << output_dir << " was NOT generated");
						return false;
					}
					base_html.close();
				}
				else
				{
					log_failure("Base site file is NOT open");
					return false;
				}
			}
			else
				log_line("Omittet file in generating: " << feed_entry.path().string());
		}
	}
	return true;
}

// > HELPFUL FUNCTIONS
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// < NEEDFUL TO RUN

/* welcome a user and show some info about
 the app: name, author and current version */
static void PrintWelcomeText()
{
	log_line("\n##################################################");
	log_line("   Welcome to " << TIM_NAME);
	log_line("   made by " << TIM_AUTHOR);
	log_line("   version " << TIM_VERSION);
	log_line("##################################################\n");
}

/* get what task to perform and on which folder (site)
 no matter how the app was opened */
static bool GetNeedeArguments(int argv, char** argc, std::string& task, std::string& name)
{
	// opened by double-click or via command line witout additional arguments
	if(argv == 1)
	{
		log_word("Type the task: ");
		std::getline(std::cin, task);
		//std::cin >> task;
		log_word("Type the name: ");
		std::getline(std::cin, name);
		//std::cin >> name;
	}
	// opened via command line with [app-task] so no need to set a name
	else if(argv == 2)
		task = argc[1];
	// opened via command line with [site-name site-task]
	else if(argv == 3)
	{
		name = argc[1];
		task = argc[2];
	}
	else
		return false;
	return true;
}

// > NEEDFUL TO RUN
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// < APP-TASKS

/* print help 'page' with essential info how to use the tool */
static void PrintHelp()
{
	log_line("##################################################");
	log_line("########### Tim Site Generator - Help ############");
	log_line("##################################################");
	log_line("Command structure: " << COMMAND_STRUCTURE);
	log_line("Possible app_tasks: " << POSSIBLE_APP_TASKS);
	log_line("Possible site_tasks: " << POSSIBLE_SITE_TASKS);
	log_line("new - creates a folder for a site with everything that is need to build the site");
	log_line("build - builds a final site, it puts all neccessery stuff into one folder with the site name");
	log_line("clean - deletes a folder with a final site");
	log_line("info - gives some information about a site");
	log_line("delete - deletes a whole folder that was created using 'new' task");
	log_line("pack - reduces sizes of final files");
	log_line("help - just prints this help");
	log_line("v - just prints the version of the app");
	log_line("todo - just prints the 'TODO' list");
}

/**/
static void PrintVersion()
{
	log_line(TIM_NAME << " version: " << TIM_VERSION);
}

/* print 'todo' list related to the app */
static void PrintTodo()
{
	log_line("\nTODO list\n");
	log_line("- add a way to build only a part of all feed to speed up the process");
	log_line("\n");
}

// > APP-TASKS
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// < SITE-TASKS

/* create a new site folder and copy an example one */
static bool NewSite(Site* site)
{
	if(std::filesystem::is_directory(site->directory, error_code))
	{
		log_failure("Site with this name already exists");
		log_error_code;
		return false;
	}

	if(!(std::filesystem::create_directory(site->directory, error_code)))
	{
		log_failure(site->directory << " directory was NOT created");
		log_error_code;
		return false;
	}

	std::filesystem::copy(std::filesystem::current_path(error_code).string() + "\\" + EXAMPLE_FOLDER, site->directory,
	                      std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive, error_code);
	log_error_code;

	return true;
}

/* build a site and put final content in its output folder */
static bool BuildSite(Site* site)
{
	if(!CheckForDirsAndFiles(site))
	{
		log_failure("Some needed directories and files are NOT valid");
		return false;
	}

	std::filesystem::remove_all(site->output_dir, error_code);
	log_error_code;
	if(!std::filesystem::create_directory(site->output_dir, error_code))
	{
		log_error_code;
		return false;
	}

	if(!ReadDataFile(site->data_file_dir, site->data))
	{
		log_failure(site->name << " site data file was NOT read");
		return false;
	}

	if(!ReadDataFile(site->config_file_dir, site->config))
	{
		log_failure(site->name << " site config file was NOT read");
		return false;
	}

	std::filesystem::copy(site->feed_dir, site->output_dir, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive, error_code);
	log_error_code;

	if(!GenerateFiles(site))
	{
		log_failure("Generating files was NOT successful");
		return false;
	}

	return true;
}

/* delete final site output folder */
static bool CleanSite(Site* site)
{
	std::filesystem::remove_all(site->output_dir, error_code);
	log_error_code;
	if(!std::filesystem::create_directory(site->output_dir, error_code))
	{
		log_error_code;
		return false;
	}
	return true;
}

/* print some information about a site */
static bool InfoSite(Site* site)
{
	if(!CheckForDirsAndFiles(site))
	{
		log_failure("Some needed directories and files are NOT valid");
		return false;
	}

	log_line("\n########## Info from _feed folder ##########");
	unsigned long long num_folders = 0, num_HTML_files = 0, num_other_files = 0;
	for(const auto& entry : std::filesystem::recursive_directory_iterator(site->feed_dir, error_code))
	{
		log_error_code;
		std::string name = entry.path().filename().string();
		if(entry.is_directory(error_code)) ++num_folders;
		else if(entry.is_regular_file(error_code))
		{
			if(entry.path().extension().string() == ".html")
				++num_HTML_files;
			else
				++num_other_files;
		}
	}
	log_line("Number of folders: " << num_folders);
	log_line("Number of HTML files: " << num_HTML_files);
	log_line("Number of other files: " << num_other_files);
	log_line("############################################\n");

	log_line("\n########## Info from _data file ##########");
	if(!ReadDataFile(site->data_file_dir, site->data))
	{
		log_failure(site->name << " site data file was NOT read");
		return false;
	}
	for(auto& data : site->data)
		log_line("### Key: " << data.first << " ### Value: " << data.second);
	log_line("############################################\n");

	log_line("\n########## Info from _cofig file ##########");
	if(!ReadDataFile(site->config_file_dir, site->config))
	{
		log_failure(site->name << " site config file was NOT read");
		return false;
	}
	for(auto& config : site->config)
		log_line("### Key: " << config.first << " ### Value: " << config.second);
	log_line("############################################\n");

	return true;
}

/* delete a whole site folder */
static bool DeleteSite(Site* site)
{
	uintmax_t num = 0;
	num           = std::filesystem::remove_all(site->directory, error_code);
	log_error_code;
	if(!num)
	{
		log_failure("Nothing was removed");
		return false;
	}
	log_success("Removed " << num << " files or directories");
	return true;
}

/* reduce sizes of files in final site folder */
static bool PackSite(Site* site)
{
	if(!ReadDataFile(site->config_file_dir, site->config))
	{
		log_failure(site->name << " site config file was NOT read");
		return false;
	}

	if(std::filesystem::is_directory(site->output_dir, error_code))
	{
		auto CheckIfContains = [=](std::vector<std::string> vec, std::string val) -> bool {
			for(std::string str : vec)
				if(str == val)
					return true;
			return false;
		};

		std::vector<std::string> vec = SplitString(site->config["to_pack"], ',');
		for(const auto& entry : std::filesystem::recursive_directory_iterator(site->output_dir, error_code))
		{
			log_error_code;

			if(entry.is_regular_file(error_code))
			{
				if(CheckIfContains(vec, entry.path().extension().string()))
				{
					std::string path     = entry.path().string();
					std::string str_data = "";

					// read from
					{
						std::ifstream file(path);
						if(file.is_open())
						{
							std::string str_line;
							while(file.peek() != EOF)
							{
								std::getline(file, str_line);
								char last_ch = '\n';
								for(char ch : str_line)
								{
									switch(ch)
									{
										case ' ':
											if(last_ch != ' ')
												str_data += ' ';
											break;
										case '\n':
											if(last_ch != '\n')
												str_data += ' ';
											break;
										default:
											str_data += ch;
											break;
									}
									last_ch = ch;
								}
							}
						}
						else
						{
							log_failure(entry.path().string() << " input file is NOT open");
							return false;
						}
					}

					// write to
					{
						std::ofstream file(path);
						if(file.is_open())
							file << str_data;
						else
						{
							log_failure(entry.path().string() << " output file is NOT open");
							return false;
						}
					}
				}
				else
					log_line("Omitted file in packing: " << entry.path().filename().string());
			}
		}
	}
	else
	{
		log_failure(site->output_dir << " directory with the site output was NOT found");
		return false;
	}

	return true;
}

// > SITE-TASKS
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// < MAIN

int main(int argv, char** argc)
{
	PrintWelcomeText();

	std::string task = "task", name = "";

	if(!GetNeedeArguments(argv, argc, task, name))
	{
		log_failure("Invalid arguments, expected command structure: " << COMMAND_STRUCTURE);
		wait;
		return -1;
	}

	// type of task: app-task
	if(!name.size())
	{
		if(task == "help")
			PrintHelp();
		else if(task == "v")
			PrintVersion();
		else if(task == "todo")
			PrintTodo();
		else
		{
			log_line("Unknown task, possible app-tasks: " << POSSIBLE_APP_TASKS);
			wait;
			return -1;
		}
	}

	// type of task: site-task
	else
	{
		std::unique_ptr<Site> site = std::make_unique<Site>();
		site->name                 = name;

		InitDirs(site.get());

		if(task == "new")
		{
			if(NewSite(site.get()))
				log_success(site->name << " has been created properly");
			else
			{
				log_failure(site->name << "has NOT been created properly");
				wait;
				return -1;
			}
		}
		else if(task == "build")
		{
			if(BuildSite(site.get()))
				log_success(site->name << " was built successfully");
			else
			{
				log_failure(site->name << " was NOT built successfully");
				wait;
				return -1;
			}
		}
		else if(task == "clean")
		{
			if(CleanSite(site.get()))
				log_success("Cleaning " << site->name << " was completed");
			else
			{
				log_failure("Cleaning " << site->name << " was NOT completed");
				wait;
				return -1;
			}
		}
		else if(task == "info")
		{
			if(!InfoSite(site.get()))
			{
				log_failure("All information about '" << site->name << "' was NOT given");
				wait;
				return -1;
			}
		}
		else if(task == "delete")
		{
			if(DeleteSite(site.get()))
				log_success(site->name << " was deleted successfully");
			else
			{
				log_failure(site->name << " was NOT deleted successfully");
				wait;
				return -1;
			}
		}
		else if(task == "pack")
		{
			if(PackSite(site.get()))
				log_success(site->name << " was packed successfully");
			else
			{
				log_failure(site->name << " was NOT packed successfully");
				wait;
				return -1;
			}
		}
		else
		{
			log_line("Unknown task, possible site-tasks: " << POSSIBLE_SITE_TASKS);
			wait;
			return -1;
		}
	}

	log_line("\n\n\napp ended with code 0");
	wait;
	return 0;
}

// > MAIN
/////////////////////////////////////////////////////
