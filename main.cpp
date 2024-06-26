#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <sstream>
#include <string.h>
#include <string_view>
#include <algorithm>

void downgradeOsuFile(std::filesystem::path filePath, bool keepOD);

std::string removeCarriageReturn(std::string str)
{
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
  return str;
}
bool is_number(const std::string &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}
std::string replaceString(std::string subject, const std::string &search, const std::string &replace)
{
  size_t pos = subject.find(search);
  while (pos != std::string::npos)
  {
    subject.replace(pos, search.length(), replace);
    pos = subject.find(search, pos + replace.length());
  }
  return subject;
}
bool string_contains(std::string input, std::string key)
{
  if (input.find(key) != std::string::npos)
  {
    return true;
  }
  return false;
}
std::vector<std::string> split(std::string s, std::string delimiter)
{
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
  {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}
std::vector<std::string> split_first(std::string s, std::string delimiter)
{
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  if ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
  {
    token = s.substr(pos_start, pos_end - pos_start);
    res.push_back(token);
    pos_start = pos_end + delim_len;
  }

  res.push_back(s.substr(pos_start));
  return res;
}

int main()
{
  int option_range = 0;
  std::cout << "##################\nosuTo2007 v1.6\nosu! : _Railgun_\nDiscord : @railgun_osu\n##################\n\n";
  std::vector<std::filesystem::path> map_list;
  for (const auto &entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
  {
    if (entry.path().extension() == ".osu")
    {
      map_list.push_back(entry.path());
    }
  }
  if (map_list.size() == 0)
  {
    std::cout << "No .osu files found\nExiting...";
    return 0;
  }
  else
  {
    std::cout << "Choose which .osu file you want to convert to v3:\n\n";
    if (map_list.size() > 1)
    {
      std::cout << "0 = All listed below\n";
    }
    for (size_t i = 1; i - 1 < map_list.size(); i++)
    {
      std::cout << i << " = " << map_list[i - 1].filename() << "\n";
      option_range++;
    }
  }
opt:
  std::cout << "Choose an option:\n";
  std::string input;
  std::cin >> input;
  if (!is_number(input))
  {
    std::cout << "Not a number\n";
    goto opt;
  }
  if (!(std::stoi(input) < option_range + 1))
  {
    std::cout << "Invalid Option\n";
    goto opt;
  }
  bool keep_OD;
opt2:
  std::cout << "Because in v3 AR(ApproachRate) is tied to OD(OverallDifficulty):\nOD = AR\nDo you prefer to have:\n1 = same OverallDifficulty (map might be hard to read)\n2 = same ApproachRate (map might be hard to acc)\n";
  std::string input2;
  std::cin >> input2;
  if (!is_number(input2))
  {
    std::cout << "Not a number\n";
    goto opt2;
  }
  if ((std::stoi(input2) < 1) || (std::stoi(input2) > 2))
  {
    std::cout << "Invalid Option\n";
    goto opt2;
  }
  if (input2 == "1")
  {
    keep_OD = true;
  }
  else
  {
    keep_OD = false;
  }
  if (std::stoi(input) == 0)
  {
    for (int i = 0; i < map_list.size(); i++)
    {
      downgradeOsuFile(map_list[i], keep_OD);
    }
  }
  else
  {
    downgradeOsuFile(map_list[std::stoi(input) - 1], keep_OD);
  }
  return 0;
}

void downgradeOsuFile(std::filesystem::path filePath, bool keepOD)
{
  // predefined keys to search for
  static std::vector<std::string> general_Var = {"AudioFilename", "PreviewTime", "SampleSet"};
  static std::vector<std::string> metaData_Var = {"Title", "Artis", "Creator", "Version"};
  static std::vector<std::string> difficulty_Var = {"HPDrainRate", "CircleSize", "OverallDifficulty", "ApproachRate", "SliderMultiplier", "SliderTickRate"};

  // current filename
  std::string fileName = "(converted) " + filePath.filename().string();

  // osu file format version
  std::string fileFormat = "";
  std::string OD_line = "";
  std::string AR_line = "";
  std::vector<std::pair<std::string, std::string>> general, metadata, difficulty;
  std::vector<std::string> events, timingPoints, hitObjects;
  bool skip = false;
  int section = 0;
  std::cout << "Downgrading " << filePath.filename() << " to v3 file format...\n";
  std::ifstream file(filePath);
  if (file.is_open())
  {
    std::string line;

    while (std::getline(file, line))
    {

      if (line.starts_with("osu file format v"))
      {
        fileFormat = replaceString(line, "osu file format v", "");
        std::cout << "Target file is version: " << fileFormat << "\n";
        if (fileFormat == "3")
        {
          std::cout << "Skipping conversion..." << "\n";
          skip = true;
          file.close();
          goto abort;
        }
      }
      if (line.starts_with("["))
      {
        // this is fine as it only gets executed once on a new section, code looks shit but it works
        if (line.starts_with("[General]"))
        {
          section = 1;
        }
        if (line.starts_with("[Editor]"))
        {
          // skip -> not present in 2007 client
          section = 0;
        }
        if (line.starts_with("[Metadata]"))
        {
          section = 2;
        }
        if (line.starts_with("[Difficulty]"))
        {
          section = 3;
        }
        if (line.starts_with("[Events]"))
        {
          section = 4;
        }
        if (line.starts_with("[TimingPoints]"))
        {
          section = 5;
        }
        if (line.starts_with("[Colours]"))
        {
          // skip -> not present in 2007 client
          section = 0;
        }
        if (line.starts_with("[HitObjects]"))
        {
          section = 6;
        }
      }

      switch (section)
      {
      case 0:
        // skip
        break;
      case 1:
        // General
        if (line != "" && !(line.starts_with(":")) && !(line.starts_with("[")))
        {
          for (int i = 0; i < general_Var.size(); i++)
          {
            if (line.starts_with(general_Var[i]))
            {
              general.push_back(std::make_pair(std::string(general_Var[i]), std::string((replaceString(line, general_Var[i] + ": ", "")))));
            }
          }
        }
        break;
      case 2:
        // Metadata
        if (line != "" && !(line.starts_with(":")) && !(line.starts_with("[")))
        {
          for (int i = 0; i < metaData_Var.size(); i++)
          {
            if (line.starts_with(metaData_Var[i] + ":"))
            {
              metadata.push_back(std::make_pair(std::string(metaData_Var[i]), std::string((replaceString(line, metaData_Var[i] + ":", "")))));
            }
          }
        }
        break;
      case 3:
        // Difficulty
        if (line != "" && !(line.starts_with(":")) && !(line.starts_with("[")))
        {
          for (int i = 0; i < difficulty_Var.size(); i++)
          {
            if (line.starts_with(difficulty_Var[i]))
            {
              //   std::cout << line << "\n";
              switch (i)
              {
              case 2:
                // skip
                OD_line = line;
                break;
              case 3:
                if (keepOD)
                {
                  // keep OD
                  difficulty.push_back(std::make_pair(std::string(difficulty_Var[2]), std::string((replaceString(OD_line, difficulty_Var[2] + ":", "")))));
                }
                else
                {
                  // overwrite OD with AR
                  difficulty.push_back(std::make_pair(std::string(difficulty_Var[2]), std::string((replaceString(line, difficulty_Var[3] + ":", "")))));
                }
                break;
              default:
                difficulty.push_back(std::make_pair(std::string(difficulty_Var[i]), std::string((replaceString(line, difficulty_Var[i] + ":", "")))));
              }
            }
          }
        }
        break;
      case 4:
        // Events
        if (line != "" && !(line.starts_with("[")) && !(line.starts_with("//")) && !line.empty())
        {

          // handle spaces from the beginning
          if (line[0] == ' ')
          {

            for (int sp = 0; sp < line.length(); sp++)
            {
              if (line[sp] == ' ')
              {
              }
              else
              {
                // first non space char found
                // number check bc b99 doesnt support storyboard fully
                std::string eChar = {line[sp]};
                if (is_number(eChar))
                {
                  // all good
                  events.push_back(line);
                }
                break;
              }
            }
          }
          else
          {
            // number check bc b99 doesnt support storyboard fully
            std::string eChar = {line[0]};
            if (!line.starts_with("//") && is_number(eChar))
            {
              events.push_back(line);
            }
          }
        }
        break;
      case 5:
        // Timing Points
        if (line != "" && !(line.starts_with("[")))
        {
          if (line.starts_with("[HitObjects]"))
          {
            section = 6;
            break;
          }
          else
          {
            if (!line.empty())
            {
              timingPoints.push_back(line);
            }
          }
        }
        break;
      case 6:
        // HitObjects
        if (line != "" && !line.starts_with("["))
        {
          hitObjects.push_back(line);
        }

        break;
      }
    }
    file.close();
  }
abort:

  if (!skip)
  {
    // process file
    std::string output = "osu file format v3\n\n[General]\n";

    for (int i = 0; i < general.size(); i++)
    {
      output += general[i].first + ": " + general[i].second + "\n";
    }

    output += "\n[Metadata]\n";

    for (int i = 0; i < metadata.size(); i++)
    {
      if (metadata[i].first == "Version")
      {
        metadata[i].second = "(Converted) " + metadata[i].second;
      }
      output += metadata[i].first + ":" + metadata[i].second + "\n";
    }

    output += "\n[Difficulty]\n";

    for (int i = 0; i < difficulty.size(); i++)
    {
      if (string_contains(difficulty[i].second, ".") && i < 3)
      {
        // 2007 client doesnt support decimal input on HP/CS/OD -> resulting in crash
        difficulty[i].second = difficulty[i].second[0];
      }

      output += difficulty[i].first + ":" + difficulty[i].second + "\n";
    }

    // Events
    output += "\n[Events]\n";
    for (int i = 0; i < events.size(); i++)
    {
      output += events[i] + "\n";
    }

    // Timing Points
    // Because v3 doesn't support sv points,
    // we need to convert those to bpm points.
    output += "\n[TimingPoints]\n";
    double currentBPM = 120.0f; // temp storing bpm for sv conversion
    double currentSV = 1.0f;    // temp storing sv
    double multiplier = 1.0f;
    for (int i = 0; i < timingPoints.size(); i++)
    {
      // std::cout << "Current TimingPoint Line: " << timingPoints[i] << "\n";
      if (split(timingPoints[i], ",").size() < 2)
      {
        //  std::cout << "This would lead to a Segmentation Fault\n";
      }
      else
      {
        bool duplicate = false;
        std::vector<std::string> v = split(removeCarriageReturn(timingPoints[i]), ",");

        if (i + 1 < timingPoints.size())
        {
          // next timing point exists.
          std::vector<std::string> v1 = split(removeCarriageReturn(timingPoints[i + 1]), ",");
          if (v[0] == v1[0])
          {
            // check if next timing point is the same time, if so, skip
            if (v1[1].starts_with("-"))
            {
              currentBPM = (((1 / std::stod(v[1])) * 1000) * 60);
              goto skipTimingPoint;    
            }
            else
            {
              // 2x timing points at same ms wtf????
              goto skipTimingPoint;
            }
          }
        }

        output += v[0] + ","; // ms
        if (v[1].starts_with("-"))
        {
          // slider velocity point
          currentSV = std::stod(replaceString(v[1], "-", ""));
          multiplier = 100 / currentSV;
          //   std::cout << "New SV Point: " << multiplier << "x | Converted BPM: ";
          if (i == 0)
          {
            //  **I assume there is no SV Point before a timing point, proof me wrong**
            //
            //  if slider velocity before timing point
            //  get the first timing point present
            //  std::string indexT = 0;
            //  for (int iii = 0; iii < timingPoints.size(); iii++)
            //  {
            //    std::vector<std::string> vT = split(timingPoints[iii], ",");
            //   if (!vT[1].starts_with("-"))
            //   {
            //      indexT = vT[1];
            //      break; // found timing point
            //    }
            //  }
            //  1/beatLength*1000*60
            //  currentBPM = (((1 / std::stof(indexT)) * 1000) * 60);
            //  output += std::to_string((currentBPM * multiplier)*1/1000/60) + "\n";
          }
          else
          {
            //  std::cout << currentBPM << "BPM -> " << (currentBPM * multiplier) << "BPM\n";
            output += std::to_string(60000 / (currentBPM * multiplier)) + "\n";
          }
        }
        else
        {
          // timing point
          output += v[1] + "\n";
          currentBPM = (((1 / std::stod(v[1])) * 1000) * 60);
          //  std::cout << "New Timing Point: " << currentBPM << "BPM\n";
        }
      skipTimingPoint:
      }
    }

    // HitObjects
    output += "\n[HitObjects]\n";
    for (int i = 0; i < hitObjects.size(); i++)
    {
      if (string_contains(hitObjects[i], "|"))
      {
        // -------------------------------------------
        // Experimental: Pre Patch: Convert "P" - Perfect Circle Slider Types to Catmull
        // -------------------------------------------

        if (string_contains(hitObjects[i], ",P|"))
        {
          hitObjects[i] = replaceString(hitObjects[i], ",P|", ",C|");
        }

        // Slider
        std::vector<std::string> v = split(hitObjects[i], ",");
        std::string Sx = v[0];
        std::string Sy = v[1];
        std::string slider = "";

        // -------------------------------------------
        // Fist Slider Patch: anchor fix
        // -------------------------------------------
        // In v3 the first anchor point must represent the starting circle of the slider.

        v[5] = v[5].insert(1, "|" + Sx + ":" + Sy);
        for (int b = 0; b < v.size(); b++)
        {
          if (b == v.size() - 1)
          {
            slider += v[b];
          }
          else
          {
            slider += v[b] + ",";
          }
        }

        // -------------------------------------------
        // Second Slider Patch: multi-type slider fix
        // -------------------------------------------
        // Because v3 doesn't support multiplie slider types in one slider...
        // Example: Bezir -> Linear -> Bezir
        // We need to duplicate the anchor point a few times at the same spot & time,
        // to "create" a linear slider inside of a bezir slider.

        std::string slider2 = "";
        std::vector<std::string> v1 = split(slider, "|");
        for (int c = 0; c < v1.size(); c++)
        {
          if (c != v1.size() - 1)
          {
            if (v1[c] == v1[c + 1])
            {
              // Red anchor points are detected by a pair of the same anchor point
              // Example ...B|142:107|208:64|208:64|186:140|...
              //                      ^^^^^^ ^^^^^^
              std::string red_anchor = "";
              for (int anc = 0; anc < 10; anc++) // should be enough?
              {
                red_anchor += v1[c] + "|";
              }
              slider2 += red_anchor;
            }
            else
            {
              slider2 += v1[c] + "|";
            }
          }
          else
          {
            slider2 += v1[c];
          }
        }

        output += slider2 + "\n";
      }
      else
      {
        // Circle, write to output as it is, as no issues with modern circle lines are found YET
        output += hitObjects[i] + "\n";
      }
    }
    std::cout << fileName << "\n";
    std::ofstream out(fileName);
    out << output;
    out.close();
    std::cout << "----------------\nDONE\n----------------\n";
  }
}